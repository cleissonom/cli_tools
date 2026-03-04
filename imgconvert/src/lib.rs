use std::ffi::OsString;
use std::fmt;
use std::path::{Path, PathBuf};

use image::{ImageError, ImageFormat};

const SUPPORTED_EXTENSIONS: [&str; 11] = [
    "png", "jpg", "jpeg", "webp", "gif", "bmp", "tiff", "tif", "ico", "pnm", "tga",
];

#[derive(Debug)]
pub enum AppError {
    InvalidExtension(String),
    UnsupportedOutputFormat(String),
    MissingInputStem {
        path: PathBuf,
    },
    SameInputOutput {
        path: PathBuf,
    },
    OutputExtensionMismatch {
        path: PathBuf,
        expected: String,
        found: String,
    },
    ReadImage {
        path: PathBuf,
        source: ImageError,
    },
    WriteImage {
        path: PathBuf,
        source: ImageError,
    },
}

impl fmt::Display for AppError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::InvalidExtension(value) => write!(
                f,
                "invalid extension '{value}'. use only letters/numbers (example: png, jpg, webp)"
            ),
            Self::UnsupportedOutputFormat(value) => write!(
                f,
                "unsupported output extension '{value}'. supported examples: {}",
                SUPPORTED_EXTENSIONS.join(", ")
            ),
            Self::MissingInputStem { path } => {
                write!(
                    f,
                    "could not derive output file name from {}",
                    path.display()
                )
            }
            Self::SameInputOutput { path } => write!(
                f,
                "input and output paths are the same ({}). choose a different extension or output path",
                path.display()
            ),
            Self::OutputExtensionMismatch {
                path,
                expected,
                found,
            } => write!(
                f,
                "output file {} has extension '{found}', but target extension is '{expected}'",
                path.display()
            ),
            Self::ReadImage { path, source } => {
                write!(f, "could not decode input image {}: {source}", path.display())
            }
            Self::WriteImage { path, source } => {
                write!(f, "could not write output image {}: {source}", path.display())
            }
        }
    }
}

impl std::error::Error for AppError {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        match self {
            Self::ReadImage { source, .. } => Some(source),
            Self::WriteImage { source, .. } => Some(source),
            _ => None,
        }
    }
}

pub fn convert_image(
    input_path: &Path,
    target_extension: &str,
    explicit_output: Option<&Path>,
) -> Result<PathBuf, AppError> {
    let normalized_target = normalize_extension(target_extension)?;
    let target_format = extension_to_format(&normalized_target)
        .ok_or_else(|| AppError::UnsupportedOutputFormat(normalized_target.clone()))?;

    validate_explicit_output_extension(explicit_output, &normalized_target, target_format)?;
    let output_path = derive_output_path(input_path, &normalized_target, explicit_output)?;

    let image = image::open(input_path).map_err(|source| AppError::ReadImage {
        path: input_path.to_path_buf(),
        source,
    })?;

    image
        .save_with_format(&output_path, target_format)
        .map_err(|source| AppError::WriteImage {
            path: output_path.clone(),
            source,
        })?;

    Ok(output_path)
}

pub fn derive_output_path(
    input_path: &Path,
    target_extension: &str,
    explicit_output: Option<&Path>,
) -> Result<PathBuf, AppError> {
    let normalized_target = normalize_extension(target_extension)?;

    let mut output_path = match explicit_output {
        Some(path) => path.to_path_buf(),
        None => {
            let stem = input_path
                .file_stem()
                .ok_or_else(|| AppError::MissingInputStem {
                    path: input_path.to_path_buf(),
                })?;

            let mut file_name = OsString::from(stem);
            file_name.push(".");
            file_name.push(&normalized_target);

            match input_path.parent() {
                Some(parent) => parent.join(file_name),
                None => PathBuf::from(file_name),
            }
        }
    };

    if explicit_output.is_some() && output_path.extension().is_none() {
        output_path.set_extension(&normalized_target);
    }

    if output_path == input_path {
        return Err(AppError::SameInputOutput { path: output_path });
    }

    Ok(output_path)
}

pub fn normalize_extension(raw: &str) -> Result<String, AppError> {
    let trimmed = raw.trim();
    let without_dot = trimmed.trim_start_matches('.');

    if without_dot.is_empty() {
        return Err(AppError::InvalidExtension(raw.to_string()));
    }

    let normalized = without_dot.to_ascii_lowercase();
    if !normalized.chars().all(|ch| ch.is_ascii_alphanumeric()) {
        return Err(AppError::InvalidExtension(raw.to_string()));
    }

    Ok(normalized)
}

fn validate_explicit_output_extension(
    explicit_output: Option<&Path>,
    normalized_target: &str,
    target_format: ImageFormat,
) -> Result<(), AppError> {
    let Some(path) = explicit_output else {
        return Ok(());
    };

    let Some(extension) = path.extension() else {
        return Ok(());
    };

    let extension_value = extension.to_string_lossy().to_string();
    let normalized_output = normalize_extension(&extension_value)?;
    let output_format = extension_to_format(&normalized_output)
        .ok_or_else(|| AppError::UnsupportedOutputFormat(normalized_output.clone()))?;

    if output_format != target_format {
        return Err(AppError::OutputExtensionMismatch {
            path: path.to_path_buf(),
            expected: normalized_target.to_string(),
            found: normalized_output,
        });
    }

    Ok(())
}

fn extension_to_format(extension: &str) -> Option<ImageFormat> {
    match extension {
        "png" => Some(ImageFormat::Png),
        "jpg" | "jpeg" => Some(ImageFormat::Jpeg),
        "webp" => Some(ImageFormat::WebP),
        "gif" => Some(ImageFormat::Gif),
        "bmp" => Some(ImageFormat::Bmp),
        "tiff" | "tif" => Some(ImageFormat::Tiff),
        "ico" => Some(ImageFormat::Ico),
        "pnm" => Some(ImageFormat::Pnm),
        "tga" => Some(ImageFormat::Tga),
        _ => None,
    }
}

#[cfg(test)]
mod tests {
    use std::fs;
    use std::path::{Path, PathBuf};
    use std::time::{SystemTime, UNIX_EPOCH};

    use image::{DynamicImage, ImageFormat, Rgba, RgbaImage};

    use super::{convert_image, derive_output_path, normalize_extension, AppError};

    #[test]
    fn normalize_extension_accepts_dotted_uppercase_input() {
        let value = normalize_extension(".JpG").expect("extension should normalize");
        assert_eq!(value, "jpg");
    }

    #[test]
    fn normalize_extension_rejects_non_alphanumeric_extension() {
        let result = normalize_extension("pn*g");
        assert!(matches!(result, Err(AppError::InvalidExtension(_))));
    }

    #[test]
    fn derive_output_path_uses_input_name_when_output_is_omitted() {
        let input = PathBuf::from("/tmp/photo.original.png");
        let output =
            derive_output_path(&input, "jpg", None).expect("output should be derived from input");

        assert_eq!(output, PathBuf::from("/tmp/photo.original.jpg"));
    }

    #[test]
    fn derive_output_path_appends_extension_when_missing() {
        let input = PathBuf::from("/tmp/source.png");
        let explicit = PathBuf::from("/tmp/converted/image_name");
        let output = derive_output_path(&input, "webp", Some(&explicit))
            .expect("extension should be appended");

        assert_eq!(output, PathBuf::from("/tmp/converted/image_name.webp"));
    }

    #[test]
    fn convert_image_creates_output_using_derived_path() {
        let input = temp_file_path("derived_input", "png");
        write_test_png(&input);

        let output = convert_image(&input, "jpg", None).expect("conversion should succeed");
        let decoded = image::open(&output).expect("output image should be decodable");

        assert_eq!(decoded.width(), 2);
        assert_eq!(decoded.height(), 2);

        cleanup_file(&input);
        cleanup_file(&output);
    }

    #[test]
    fn convert_image_rejects_same_input_and_output_path() {
        let input = temp_file_path("same_path", "png");
        write_test_png(&input);

        let result = convert_image(&input, "png", None);
        assert!(matches!(result, Err(AppError::SameInputOutput { .. })));

        cleanup_file(&input);
    }

    #[test]
    fn convert_image_rejects_mismatched_output_extension() {
        let input = temp_file_path("mismatch_input", "png");
        let output = temp_file_path("mismatch_output", "png");
        write_test_png(&input);

        let result = convert_image(&input, "jpg", Some(&output));
        assert!(matches!(
            result,
            Err(AppError::OutputExtensionMismatch { .. })
        ));

        cleanup_file(&input);
        cleanup_file(&output);
    }

    fn write_test_png(path: &Path) {
        let mut image = RgbaImage::new(2, 2);
        for pixel in image.pixels_mut() {
            *pixel = Rgba([255, 64, 32, 255]);
        }

        DynamicImage::ImageRgba8(image)
            .save_with_format(path, ImageFormat::Png)
            .expect("test image should be written");
    }

    fn temp_file_path(label: &str, extension: &str) -> PathBuf {
        let now = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .expect("clock should be after unix epoch")
            .as_nanos();
        let pid = std::process::id();

        std::env::temp_dir().join(format!("imgconvert_{label}_{pid}_{now}.{extension}"))
    }

    fn cleanup_file(path: &Path) {
        if path.exists() {
            fs::remove_file(path).expect("failed to remove temp file");
        }
    }
}
