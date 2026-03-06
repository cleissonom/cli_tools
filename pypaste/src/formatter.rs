#[derive(Clone, Copy, Debug, Eq, PartialEq)]
enum TripleQuote {
    Single,
    Double,
}

impl TripleQuote {
    fn from_byte(byte: u8) -> Self {
        if byte == b'\'' {
            Self::Single
        } else {
            Self::Double
        }
    }

    fn as_byte(self) -> u8 {
        match self {
            Self::Single => b'\'',
            Self::Double => b'"',
        }
    }
}

#[derive(Debug)]
struct LineInfo {
    text: String,
    eol: String,
    start_in_triple: bool,
    is_blank: bool,
    collapsible_blank: bool,
    indent: usize,
}

pub fn compact_python_script(input: &str) -> String {
    let raw_lines = split_lines_preserve_eol(input);
    if raw_lines.is_empty() {
        return String::new();
    }

    let mut triple_state: Option<TripleQuote> = None;
    let mut lines = Vec::with_capacity(raw_lines.len());

    for (raw_line, eol) in raw_lines {
        let start_in_triple = triple_state.is_some();
        triple_state = update_triple_state(&raw_line, triple_state);
        let end_in_triple = triple_state.is_some();

        let text = if start_in_triple || end_in_triple {
            raw_line
        } else {
            trim_trailing_spaces_tabs(&raw_line).to_string()
        };

        let is_blank = text.is_empty();
        let collapsible_blank = is_blank && !start_in_triple && !end_in_triple;
        let indent = if is_blank {
            0
        } else {
            indentation_columns(&text)
        };

        lines.push(LineInfo {
            text,
            eol,
            start_in_triple,
            is_blank,
            collapsible_blank,
            indent,
        });
    }

    let prev_code_anchor = build_prev_code_anchor(&lines);
    let next_code_anchor = build_next_code_anchor(&lines);

    let mut output = String::new();
    let mut has_written_nonblank = false;
    let mut last_written_blank = false;

    for (idx, line) in lines.iter().enumerate() {
        if line.collapsible_blank {
            if !should_keep_collapsible_blank(idx, &lines, &prev_code_anchor, &next_code_anchor) {
                continue;
            }

            if !has_written_nonblank || last_written_blank {
                continue;
            }

            output.push_str(&line.eol);
            last_written_blank = true;
            continue;
        }

        output.push_str(&line.text);
        output.push_str(&line.eol);

        if !line.is_blank {
            has_written_nonblank = true;
        }

        last_written_blank = line.is_blank;
    }

    output
}

fn build_prev_code_anchor(lines: &[LineInfo]) -> Vec<Option<usize>> {
    let mut output = vec![None; lines.len()];
    let mut last_anchor = None;

    for (idx, line) in lines.iter().enumerate() {
        output[idx] = last_anchor;
        if is_code_anchor(line) {
            last_anchor = Some(idx);
        }
    }

    output
}

fn build_next_code_anchor(lines: &[LineInfo]) -> Vec<Option<usize>> {
    let mut output = vec![None; lines.len()];
    let mut next_anchor = None;

    for idx in (0..lines.len()).rev() {
        output[idx] = next_anchor;
        if is_code_anchor(&lines[idx]) {
            next_anchor = Some(idx);
        }
    }

    output
}

fn should_keep_collapsible_blank(
    idx: usize,
    lines: &[LineInfo],
    prev_code_anchor: &[Option<usize>],
    next_code_anchor: &[Option<usize>],
) -> bool {
    let Some(prev_idx) = prev_code_anchor[idx] else {
        return false;
    };

    let Some(next_idx) = next_code_anchor[idx] else {
        return false;
    };

    lines[prev_idx].indent == 0 || lines[next_idx].indent == 0
}

fn is_code_anchor(line: &LineInfo) -> bool {
    !line.is_blank && !line.start_in_triple
}

fn trim_trailing_spaces_tabs(line: &str) -> &str {
    line.trim_end_matches([' ', '\t'])
}

fn indentation_columns(line: &str) -> usize {
    line.bytes()
        .take_while(|byte| *byte == b' ' || *byte == b'\t')
        .map(|byte| if byte == b'\t' { 4 } else { 1 })
        .sum()
}

fn split_lines_preserve_eol(input: &str) -> Vec<(String, String)> {
    let bytes = input.as_bytes();
    let mut lines = Vec::new();
    let mut start = 0usize;
    let mut idx = 0usize;

    while idx < bytes.len() {
        if bytes[idx] == b'\n' {
            if idx > start && bytes[idx - 1] == b'\r' {
                lines.push((input[start..idx - 1].to_string(), "\r\n".to_string()));
            } else {
                lines.push((input[start..idx].to_string(), "\n".to_string()));
            }

            start = idx + 1;
        }

        idx += 1;
    }

    if start < input.len() {
        lines.push((input[start..].to_string(), String::new()));
    }

    lines
}

fn update_triple_state(line: &str, mut state: Option<TripleQuote>) -> Option<TripleQuote> {
    let bytes = line.as_bytes();
    let mut idx = 0usize;
    let mut short_string_quote: Option<u8> = None;

    while idx < bytes.len() {
        if let Some(triple_quote) = state {
            let quote = triple_quote.as_byte();
            if is_triple_quote_at(bytes, idx, quote) && !is_escaped(bytes, idx) {
                state = None;
                idx += 3;
                continue;
            }

            idx += 1;
            continue;
        }

        if let Some(quote) = short_string_quote {
            if bytes[idx] == b'\\' {
                idx += 1;
                if idx < bytes.len() {
                    idx += 1;
                }
                continue;
            }

            if bytes[idx] == quote && !is_escaped(bytes, idx) {
                short_string_quote = None;
            }

            idx += 1;
            continue;
        }

        match bytes[idx] {
            b'#' => break,
            b'\'' | b'"' => {
                let quote = bytes[idx];
                if is_triple_quote_at(bytes, idx, quote) && !is_escaped(bytes, idx) {
                    state = Some(TripleQuote::from_byte(quote));
                    idx += 3;
                } else {
                    short_string_quote = Some(quote);
                    idx += 1;
                }
            }
            _ => idx += 1,
        }
    }

    state
}

fn is_triple_quote_at(bytes: &[u8], idx: usize, quote: u8) -> bool {
    idx + 2 < bytes.len()
        && bytes[idx] == quote
        && bytes[idx + 1] == quote
        && bytes[idx + 2] == quote
}

fn is_escaped(bytes: &[u8], idx: usize) -> bool {
    if idx == 0 {
        return false;
    }

    let mut backslashes = 0usize;
    let mut cursor = idx;

    while cursor > 0 {
        cursor -= 1;
        if bytes[cursor] == b'\\' {
            backslashes += 1;
        } else {
            break;
        }
    }

    backslashes % 2 == 1
}

#[cfg(test)]
mod tests {
    use super::compact_python_script;

    #[test]
    fn compacts_user_example_and_preserves_blocks() {
        let input = "import os\n\ndef main():\n    print(\"Hello, World!\")\n\n    if True:\n      print(\"Hello, World!\")\n    else:\n      print(\"Hello, World!\")\n\n    try:\n      print(\"Hello, World!\")\n    except Exception as e:\n      print(e)\n\n    return 0\n";

        let expected = "import os\n\ndef main():\n    print(\"Hello, World!\")\n    if True:\n      print(\"Hello, World!\")\n    else:\n      print(\"Hello, World!\")\n    try:\n      print(\"Hello, World!\")\n    except Exception as e:\n      print(e)\n    return 0\n";

        assert_eq!(compact_python_script(input), expected);
    }

    #[test]
    fn keeps_top_level_separation() {
        let input = "\n\nimport os\n\n\n\ndef main():\n    return 0\n\n";
        let expected = "import os\n\ndef main():\n    return 0\n";

        assert_eq!(compact_python_script(input), expected);
    }

    #[test]
    fn preserves_blank_lines_inside_multiline_string() {
        let input =
            "def main():\n    text = \"\"\"line one\n\nline three\"\"\"\n\n    return text\n";
        let expected =
            "def main():\n    text = \"\"\"line one\n\nline three\"\"\"\n    return text\n";

        assert_eq!(compact_python_script(input), expected);
    }

    #[test]
    fn trims_trailing_spaces_tabs_on_code_lines() {
        let input = "import os   \n\ndef main():\t\t\n    return 1    \t\n";
        let expected = "import os\n\ndef main():\n    return 1\n";

        assert_eq!(compact_python_script(input), expected);
    }

    #[test]
    fn does_not_toggle_triple_state_for_comment_markers() {
        let input = "# \"\"\" comment marker\n\nprint('ok')\n";
        let expected = "# \"\"\" comment marker\n\nprint('ok')\n";

        assert_eq!(compact_python_script(input), expected);
    }

    #[test]
    fn compaction_is_idempotent() {
        let input = "import os\n\ndef main():\n    print('a')\n\n\n    return 0\n";

        let once = compact_python_script(input);
        let twice = compact_python_script(&once);

        assert_eq!(once, twice);
    }
}
