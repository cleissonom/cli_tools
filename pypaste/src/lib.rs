pub mod clipboard;
pub mod formatter;

use std::fmt;
use std::fs;
use std::path::{Path, PathBuf};

use clipboard::ClipboardError;

#[derive(Debug)]
pub enum AppError {
    ReadFile {
        path: PathBuf,
        source: std::io::Error,
    },
    InvalidUtf8 {
        path: PathBuf,
        source: std::string::FromUtf8Error,
    },
    Clipboard(ClipboardError),
}

impl fmt::Display for AppError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::ReadFile { path, source } => {
                write!(f, "could not read {}: {source}", path.display())
            }
            Self::InvalidUtf8 { path, .. } => {
                write!(f, "file {} is not valid UTF-8", path.display())
            }
            Self::Clipboard(source) => write!(f, "clipboard error: {source}"),
        }
    }
}

impl std::error::Error for AppError {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        match self {
            Self::ReadFile { source, .. } => Some(source),
            Self::InvalidUtf8 { source, .. } => Some(source),
            Self::Clipboard(source) => Some(source),
        }
    }
}

pub fn compact_source(source: &str) -> String {
    formatter::compact_python_script(source)
}

pub fn compact_file(path: &Path) -> Result<String, AppError> {
    let bytes = fs::read(path).map_err(|source| AppError::ReadFile {
        path: path.to_path_buf(),
        source,
    })?;

    let source = String::from_utf8(bytes).map_err(|source| AppError::InvalidUtf8 {
        path: path.to_path_buf(),
        source,
    })?;

    Ok(compact_source(&source))
}

pub fn compact_file_to_clipboard(path: &Path) -> Result<String, AppError> {
    let compacted = compact_file(path)?;
    clipboard::copy_to_clipboard(&compacted).map_err(AppError::Clipboard)?;
    Ok(compacted)
}

#[cfg(test)]
mod tests {
    use std::fs;
    use std::path::PathBuf;
    use std::time::{SystemTime, UNIX_EPOCH};

    use super::{compact_file, AppError};

    #[test]
    fn compact_file_returns_compacted_content() {
        let path = temp_file_path("compact_file_valid");

        fs::write(&path, b"import os\n\n\ndef main():\n    return 0\n\n")
            .expect("failed to write temp file");

        let output = compact_file(&path).expect("expected compacted output");
        fs::remove_file(&path).expect("failed to cleanup temp file");

        assert_eq!(output, "import os\n\ndef main():\n    return 0\n");
    }

    #[test]
    fn compact_file_rejects_invalid_utf8() {
        let path = temp_file_path("compact_file_invalid_utf8");

        fs::write(&path, [0xff, 0xfe, 0xfd]).expect("failed to write invalid utf8 file");

        let result = compact_file(&path);
        fs::remove_file(&path).expect("failed to cleanup temp file");

        assert!(matches!(result, Err(AppError::InvalidUtf8 { .. })));
    }

    fn temp_file_path(label: &str) -> PathBuf {
        let now = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .expect("clock should be after unix epoch")
            .as_nanos();

        let pid = std::process::id();
        std::env::temp_dir().join(format!("pypaste_{label}_{pid}_{now}.py"))
    }
}
