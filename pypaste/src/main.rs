use std::env;
use std::path::PathBuf;

use pypaste::{compact_file, compact_file_to_clipboard, AppError};

#[derive(Debug)]
struct CliArgs {
    script_path: PathBuf,
    stdout_only: bool,
}

#[derive(Debug)]
enum ParseError {
    Help(String),
    Message(String),
}

fn main() {
    match parse_args(env::args()) {
        Ok(args) => {
            if let Err(error) = execute(args) {
                eprintln!("Error: {error}");
                std::process::exit(1);
            }
        }
        Err(ParseError::Help(help)) => {
            println!("{help}");
        }
        Err(ParseError::Message(message)) => {
            eprintln!("Error: {message}");
            eprintln!("{}", usage("pypaste"));
            std::process::exit(1);
        }
    }
}

fn execute(args: CliArgs) -> Result<(), AppError> {
    if args.stdout_only {
        let compacted = compact_file(&args.script_path)?;
        print!("{compacted}");
        return Ok(());
    }

    compact_file_to_clipboard(&args.script_path)?;
    println!(
        "Copied compacted Python script to clipboard from {}",
        args.script_path.display()
    );

    Ok(())
}

fn parse_args<I>(args: I) -> Result<CliArgs, ParseError>
where
    I: IntoIterator<Item = String>,
{
    let mut args = args.into_iter();
    let executable = args.next().unwrap_or_else(|| "pypaste".to_string());

    let mut script_path: Option<PathBuf> = None;
    let mut stdout_only = false;

    for arg in args {
        match arg.as_str() {
            "-h" | "--help" => return Err(ParseError::Help(usage(&executable))),
            "--stdout" => stdout_only = true,
            _ if arg.starts_with('-') => {
                return Err(ParseError::Message(format!("unknown option: {arg}")));
            }
            _ => {
                if script_path.is_some() {
                    return Err(ParseError::Message(
                        "expected a single python script path".to_string(),
                    ));
                }

                script_path = Some(PathBuf::from(arg));
            }
        }
    }

    let Some(script_path) = script_path else {
        return Err(ParseError::Message(
            "missing python script path".to_string(),
        ));
    };

    Ok(CliArgs {
        script_path,
        stdout_only,
    })
}

fn usage(executable: &str) -> String {
    format!(
        "Usage: {executable} [--stdout] <script.py>\n\n\
Compacts redundant Python blank lines/whitespace while preserving code blocks and\n\
copies the resulting script to your clipboard.\n\n\
Options:\n  --stdout  Print compacted output instead of copying to clipboard\n  -h, --help  Show this message"
    )
}

#[cfg(test)]
mod tests {
    use super::{parse_args, ParseError};

    #[test]
    fn parse_args_accepts_stdout_option() {
        let args = vec![
            "pypaste".to_string(),
            "--stdout".to_string(),
            "script.py".to_string(),
        ];

        let parsed = parse_args(args).expect("args should parse");

        assert!(parsed.stdout_only);
        assert_eq!(parsed.script_path.to_string_lossy(), "script.py");
    }

    #[test]
    fn parse_args_requires_script_path() {
        let args = vec!["pypaste".to_string()];

        let parsed = parse_args(args);

        assert!(matches!(parsed, Err(ParseError::Message(_))));
    }

    #[test]
    fn parse_args_rejects_unknown_option() {
        let args = vec![
            "pypaste".to_string(),
            "--unknown".to_string(),
            "script.py".to_string(),
        ];

        let parsed = parse_args(args);

        assert!(matches!(parsed, Err(ParseError::Message(_))));
    }
}
