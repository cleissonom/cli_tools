use std::fmt;
use std::io::Write;
use std::process::{Command, Stdio};

#[derive(Debug)]
pub enum ClipboardError {
    UnsupportedPlatform,
    ClipboardUnavailable(String),
    ClipboardWriteFailed(String),
}

impl fmt::Display for ClipboardError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::UnsupportedPlatform => write!(
                f,
                "clipboard integration is not supported on this platform for pypaste"
            ),
            Self::ClipboardUnavailable(message) => write!(f, "{message}"),
            Self::ClipboardWriteFailed(message) => write!(f, "{message}"),
        }
    }
}

impl std::error::Error for ClipboardError {}

#[derive(Clone, Copy, Debug)]
struct ClipboardCommand {
    program: &'static str,
    args: &'static [&'static str],
}

#[cfg(target_os = "macos")]
const PLATFORM_COMMANDS: [ClipboardCommand; 1] = [ClipboardCommand {
    program: "pbcopy",
    args: &[],
}];

#[cfg(target_os = "linux")]
const PLATFORM_COMMANDS: [ClipboardCommand; 3] = [
    ClipboardCommand {
        program: "wl-copy",
        args: &[],
    },
    ClipboardCommand {
        program: "xclip",
        args: &["-selection", "clipboard"],
    },
    ClipboardCommand {
        program: "xsel",
        args: &["--clipboard", "--input"],
    },
];

#[cfg(target_os = "windows")]
const PLATFORM_COMMANDS: [ClipboardCommand; 1] = [ClipboardCommand {
    program: "clip",
    args: &[],
}];

#[cfg(not(any(target_os = "macos", target_os = "linux", target_os = "windows")))]
const PLATFORM_COMMANDS: [ClipboardCommand; 0] = [];

enum RunError {
    NotFound,
    Failed(String),
}

pub fn copy_to_clipboard(text: &str) -> Result<(), ClipboardError> {
    if PLATFORM_COMMANDS.is_empty() {
        return Err(ClipboardError::UnsupportedPlatform);
    }

    let mut missing_commands = Vec::new();
    let mut failures = Vec::new();

    for command in PLATFORM_COMMANDS {
        match run_clipboard_command(command, text) {
            Ok(()) => return Ok(()),
            Err(RunError::NotFound) => missing_commands.push(command.program.to_string()),
            Err(RunError::Failed(message)) => {
                failures.push(format!("{} failed: {message}", command.program));
            }
        }
    }

    if missing_commands.len() == PLATFORM_COMMANDS.len() {
        return Err(ClipboardError::ClipboardUnavailable(format!(
            "could not find a clipboard command. tried: {}",
            missing_commands.join(", ")
        )));
    }

    if !failures.is_empty() {
        return Err(ClipboardError::ClipboardWriteFailed(format!(
            "could not write to clipboard. {}",
            failures.join("; ")
        )));
    }

    Err(ClipboardError::ClipboardUnavailable(
        "clipboard command was not available".to_string(),
    ))
}

fn run_clipboard_command(command: ClipboardCommand, text: &str) -> Result<(), RunError> {
    let mut child = Command::new(command.program)
        .args(command.args)
        .stdin(Stdio::piped())
        .stdout(Stdio::null())
        .stderr(Stdio::null())
        .spawn()
        .map_err(|error| {
            if error.kind() == std::io::ErrorKind::NotFound {
                RunError::NotFound
            } else {
                RunError::Failed(error.to_string())
            }
        })?;

    if let Some(mut stdin) = child.stdin.take() {
        stdin
            .write_all(text.as_bytes())
            .map_err(|error| RunError::Failed(error.to_string()))?;
    }

    let status = child
        .wait()
        .map_err(|error| RunError::Failed(error.to_string()))?;

    if status.success() {
        Ok(())
    } else {
        Err(RunError::Failed(format!("exit status: {status}")))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn reports_error_when_no_commands_are_available() {
        if PLATFORM_COMMANDS.is_empty() {
            let result = copy_to_clipboard("print('ok')");
            assert!(matches!(result, Err(ClipboardError::UnsupportedPlatform)));
            return;
        }

        assert!(!PLATFORM_COMMANDS.is_empty());
    }
}
