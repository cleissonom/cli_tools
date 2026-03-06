use std::ffi::OsString;
use std::fmt;
use std::fs;
use std::io;
use std::path::{Component, Path, PathBuf};

use image::{imageops::FilterType, DynamicImage, ImageError, ImageFormat};

const SUPPORTED_IMAGE_EXTENSIONS: [&str; 10] = [
    "png", "jpg", "jpeg", "webp", "gif", "bmp", "tiff", "tif", "ico", "tga",
];

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Size {
    pub width: u32,
    pub height: u32,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum HorizontalFrame {
    Left,
    Center,
    Right,
}

impl HorizontalFrame {
    pub fn parse(raw: &str) -> Result<Self, AppError> {
        match raw.trim().to_ascii_lowercase().as_str() {
            "left" => Ok(Self::Left),
            "center" => Ok(Self::Center),
            "right" => Ok(Self::Right),
            _ => Err(AppError::InvalidHorizontalFrame(raw.to_string())),
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum VerticalFrame {
    Top,
    Center,
    Bottom,
}

impl VerticalFrame {
    pub fn parse(raw: &str) -> Result<Self, AppError> {
        match raw.trim().to_ascii_lowercase().as_str() {
            "top" => Ok(Self::Top),
            "center" => Ok(Self::Center),
            "bottom" => Ok(Self::Bottom),
            _ => Err(AppError::InvalidVerticalFrame(raw.to_string())),
        }
    }
}

#[derive(Debug)]
pub enum AppError {
    InvalidSize(String),
    InvalidHorizontalFrame(String),
    InvalidVerticalFrame(String),
    CurrentDirectory {
        source: io::Error,
    },
    ReadDirectory {
        path: PathBuf,
        source: io::Error,
    },
    MissingInputPath {
        cwd: PathBuf,
    },
    AmbiguousInputPath {
        cwd: PathBuf,
        candidates: Vec<PathBuf>,
    },
    MissingInputStem {
        path: PathBuf,
    },
    MissingInputExtension {
        path: PathBuf,
    },
    SameInputOutput {
        input: PathBuf,
        output: PathBuf,
    },
    UnsupportedOutputExtension {
        path: PathBuf,
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
            Self::InvalidSize(value) => write!(
                f,
                "invalid --size '{value}'. expected WIDTHxHEIGHT with positive integers (example: 800x600)"
            ),
            Self::InvalidHorizontalFrame(value) => write!(
                f,
                "invalid --frame-horizontal '{value}'. expected one of: left, center, right"
            ),
            Self::InvalidVerticalFrame(value) => write!(
                f,
                "invalid --frame-vertical '{value}'. expected one of: top, center, bottom"
            ),
            Self::CurrentDirectory { source } => {
                write!(f, "could not determine current directory: {source}")
            }
            Self::ReadDirectory { path, source } => {
                write!(f, "could not read directory {}: {source}", path.display())
            }
            Self::MissingInputPath { cwd } => write!(
                f,
                "--input was not provided and no supported image files were found in {}. provide --input <PATH>",
                cwd.display()
            ),
            Self::AmbiguousInputPath { cwd, candidates } => {
                let preview: Vec<String> = candidates
                    .iter()
                    .take(5)
                    .map(|path| path.display().to_string())
                    .collect();
                let suffix = if candidates.len() > 5 { ", ..." } else { "" };

                write!(
                    f,
                    "--input was not provided and multiple supported image files were found in {}: {}{}. provide --input <PATH>",
                    cwd.display(),
                    preview.join(", "),
                    suffix
                )
            }
            Self::MissingInputStem { path } => write!(
                f,
                "could not derive output filename stem from input path {}",
                path.display()
            ),
            Self::MissingInputExtension { path } => write!(
                f,
                "could not derive output extension from input path {}. use an input file with an extension or pass --output with one",
                path.display()
            ),
            Self::SameInputOutput { input, output } => write!(
                f,
                "input and output paths resolve to the same file (input: {}, output: {}). choose a different --output path",
                input.display(),
                output.display()
            ),
            Self::UnsupportedOutputExtension { path } => write!(
                f,
                "output file {} has an unsupported or missing extension. supported examples: {}",
                path.display(),
                SUPPORTED_IMAGE_EXTENSIONS.join(", ")
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
            Self::CurrentDirectory { source } => Some(source),
            Self::ReadDirectory { source, .. } => Some(source),
            Self::ReadImage { source, .. } => Some(source),
            Self::WriteImage { source, .. } => Some(source),
            _ => None,
        }
    }
}

#[derive(Debug, Clone, Copy)]
pub struct CropRequest<'a> {
    pub size: Size,
    pub input_path: Option<&'a Path>,
    pub keep_proportion: bool,
    pub frame_horizontal: HorizontalFrame,
    pub frame_vertical: VerticalFrame,
    pub output_path: Option<&'a Path>,
}

pub fn parse_size(raw: &str) -> Result<Size, AppError> {
    let trimmed = raw.trim();
    let normalized = trimmed.to_ascii_lowercase();
    let parts: Vec<&str> = normalized.split('x').collect();

    if parts.len() != 2 || parts[0].is_empty() || parts[1].is_empty() {
        return Err(AppError::InvalidSize(raw.to_string()));
    }

    let width = parts[0]
        .parse::<u32>()
        .map_err(|_| AppError::InvalidSize(raw.to_string()))?;
    let height = parts[1]
        .parse::<u32>()
        .map_err(|_| AppError::InvalidSize(raw.to_string()))?;

    if width == 0 || height == 0 {
        return Err(AppError::InvalidSize(raw.to_string()));
    }

    Ok(Size { width, height })
}

pub fn resolve_input_path(explicit_input: Option<&Path>, cwd: &Path) -> Result<PathBuf, AppError> {
    if let Some(path) = explicit_input {
        return Ok(path.to_path_buf());
    }

    let mut candidates = Vec::new();
    let entries = fs::read_dir(cwd).map_err(|source| AppError::ReadDirectory {
        path: cwd.to_path_buf(),
        source,
    })?;

    for entry in entries {
        let entry = entry.map_err(|source| AppError::ReadDirectory {
            path: cwd.to_path_buf(),
            source,
        })?;

        let path = entry.path();
        if !path.is_file() {
            continue;
        }

        if is_supported_image_extension(&path) {
            candidates.push(path);
        }
    }

    candidates.sort();

    match candidates.len() {
        0 => Err(AppError::MissingInputPath {
            cwd: cwd.to_path_buf(),
        }),
        1 => Ok(candidates.remove(0)),
        _ => Err(AppError::AmbiguousInputPath {
            cwd: cwd.to_path_buf(),
            candidates,
        }),
    }
}

pub fn derive_output_path(
    input_path: &Path,
    size: Size,
    explicit_output: Option<&Path>,
) -> Result<PathBuf, AppError> {
    let input_stem = input_path
        .file_stem()
        .ok_or_else(|| AppError::MissingInputStem {
            path: input_path.to_path_buf(),
        })?;
    let input_extension =
        input_path
            .extension()
            .ok_or_else(|| AppError::MissingInputExtension {
                path: input_path.to_path_buf(),
            })?;

    let mut output_path = match explicit_output {
        Some(path) => path.to_path_buf(),
        None => {
            let mut file_name = OsString::from(input_stem);
            file_name.push(format!(".{}x{}.", size.width, size.height));
            file_name.push(input_extension);

            match input_path.parent() {
                Some(parent) => parent.join(file_name),
                None => PathBuf::from(file_name),
            }
        }
    };

    if explicit_output.is_some() && output_path.extension().is_none() {
        output_path.set_extension(input_extension);
    }

    Ok(output_path)
}

pub fn ensure_distinct_paths(
    input_path: &Path,
    output_path: &Path,
    cwd: &Path,
) -> Result<(), AppError> {
    if paths_collide(input_path, output_path, cwd) {
        return Err(AppError::SameInputOutput {
            input: input_path.to_path_buf(),
            output: output_path.to_path_buf(),
        });
    }

    Ok(())
}

pub fn crop_image(request: CropRequest<'_>) -> Result<PathBuf, AppError> {
    let cwd = std::env::current_dir().map_err(|source| AppError::CurrentDirectory { source })?;
    crop_image_in_dir(request, &cwd)
}

pub fn crop_image_in_dir(request: CropRequest<'_>, cwd: &Path) -> Result<PathBuf, AppError> {
    let input_path = resolve_input_path(request.input_path, cwd)?;
    let output_path = derive_output_path(&input_path, request.size, request.output_path)?;
    ensure_distinct_paths(&input_path, &output_path, cwd)?;

    let output_format = output_format_from_path(&output_path).ok_or_else(|| {
        AppError::UnsupportedOutputExtension {
            path: output_path.clone(),
        }
    })?;

    let image = image::open(&input_path).map_err(|source| AppError::ReadImage {
        path: input_path.clone(),
        source,
    })?;

    let processed = crop_to_target(
        image,
        request.size,
        request.keep_proportion,
        request.frame_horizontal,
        request.frame_vertical,
    );

    processed
        .save_with_format(&output_path, output_format)
        .map_err(|source| AppError::WriteImage {
            path: output_path.clone(),
            source,
        })?;

    Ok(output_path)
}

fn crop_to_target(
    image: DynamicImage,
    target: Size,
    keep_proportion: bool,
    frame_horizontal: HorizontalFrame,
    frame_vertical: VerticalFrame,
) -> DynamicImage {
    if !keep_proportion {
        return image.resize_exact(target.width, target.height, FilterType::Lanczos3);
    }

    let source_width = image.width();
    let source_height = image.height();

    let (resize_width, resize_height) =
        cover_dimensions(source_width, source_height, target.width, target.height);

    let resized = if resize_width == source_width && resize_height == source_height {
        image
    } else {
        image.resize_exact(resize_width, resize_height, FilterType::Lanczos3)
    };

    let x = horizontal_offset(resize_width, target.width, frame_horizontal);
    let y = vertical_offset(resize_height, target.height, frame_vertical);

    resized.crop_imm(x, y, target.width, target.height)
}

fn cover_dimensions(
    source_width: u32,
    source_height: u32,
    target_width: u32,
    target_height: u32,
) -> (u32, u32) {
    let sw = u128::from(source_width);
    let sh = u128::from(source_height);
    let tw = u128::from(target_width);
    let th = u128::from(target_height);

    if sw * th >= sh * tw {
        let resize_width = (sw * th).div_ceil(sh) as u32;
        (resize_width, target_height)
    } else {
        let resize_height = (sh * tw).div_ceil(sw) as u32;
        (target_width, resize_height)
    }
}

fn horizontal_offset(current_width: u32, target_width: u32, frame: HorizontalFrame) -> u32 {
    let diff = current_width.saturating_sub(target_width);
    match frame {
        HorizontalFrame::Left => 0,
        HorizontalFrame::Center => diff / 2,
        HorizontalFrame::Right => diff,
    }
}

fn vertical_offset(current_height: u32, target_height: u32, frame: VerticalFrame) -> u32 {
    let diff = current_height.saturating_sub(target_height);
    match frame {
        VerticalFrame::Top => 0,
        VerticalFrame::Center => diff / 2,
        VerticalFrame::Bottom => diff,
    }
}

fn is_supported_image_extension(path: &Path) -> bool {
    let Some(extension) = normalized_extension(path) else {
        return false;
    };

    SUPPORTED_IMAGE_EXTENSIONS
        .iter()
        .any(|supported| extension == *supported)
}

fn output_format_from_path(path: &Path) -> Option<ImageFormat> {
    let extension = normalized_extension(path)?;

    match extension.as_str() {
        "png" => Some(ImageFormat::Png),
        "jpg" | "jpeg" => Some(ImageFormat::Jpeg),
        "webp" => Some(ImageFormat::WebP),
        "gif" => Some(ImageFormat::Gif),
        "bmp" => Some(ImageFormat::Bmp),
        "tiff" | "tif" => Some(ImageFormat::Tiff),
        "ico" => Some(ImageFormat::Ico),
        "tga" => Some(ImageFormat::Tga),
        _ => None,
    }
}

fn normalized_extension(path: &Path) -> Option<String> {
    Some(path.extension()?.to_string_lossy().to_ascii_lowercase())
}

fn paths_collide(input_path: &Path, output_path: &Path, cwd: &Path) -> bool {
    let input_absolute = normalize_absolute_path(input_path, cwd);
    let output_absolute = normalize_absolute_path(output_path, cwd);

    match (
        input_absolute.canonicalize(),
        output_absolute.canonicalize(),
    ) {
        (Ok(left), Ok(right)) => left == right,
        (Ok(left), Err(_)) => left == output_absolute,
        (Err(_), Ok(right)) => input_absolute == right,
        (Err(_), Err(_)) => input_absolute == output_absolute,
    }
}

fn normalize_absolute_path(path: &Path, cwd: &Path) -> PathBuf {
    let absolute = if path.is_absolute() {
        path.to_path_buf()
    } else {
        cwd.join(path)
    };

    let mut normalized = PathBuf::new();
    for component in absolute.components() {
        match component {
            Component::CurDir => {}
            Component::ParentDir => {
                normalized.pop();
            }
            Component::Normal(part) => normalized.push(part),
            Component::RootDir => normalized.push(component.as_os_str()),
            Component::Prefix(prefix) => normalized.push(prefix.as_os_str()),
        }
    }

    normalized
}

#[cfg(test)]
mod tests {
    use std::fs;
    use std::path::{Path, PathBuf};
    use std::time::{SystemTime, UNIX_EPOCH};

    use image::{DynamicImage, GenericImageView, ImageFormat, Rgba, RgbaImage};

    use super::{
        crop_image_in_dir, derive_output_path, ensure_distinct_paths, parse_size,
        resolve_input_path, AppError, CropRequest, HorizontalFrame, Size, VerticalFrame,
    };

    #[test]
    fn parse_size_accepts_valid_dimensions() {
        let size = parse_size("800x600").expect("size should parse");
        assert_eq!(
            size,
            Size {
                width: 800,
                height: 600
            }
        );
    }

    #[test]
    fn parse_size_rejects_invalid_format() {
        let result = parse_size("800-600");
        assert!(matches!(result, Err(AppError::InvalidSize(_))));
    }

    #[test]
    fn parse_size_rejects_zero_values() {
        let result = parse_size("0x600");
        assert!(matches!(result, Err(AppError::InvalidSize(_))));
    }

    #[test]
    fn frame_parser_validates_options() {
        assert_eq!(
            HorizontalFrame::parse("Left").expect("frame should parse"),
            HorizontalFrame::Left
        );
        assert_eq!(
            VerticalFrame::parse("BOTTOM").expect("frame should parse"),
            VerticalFrame::Bottom
        );

        assert!(matches!(
            HorizontalFrame::parse("side"),
            Err(AppError::InvalidHorizontalFrame(_))
        ));
        assert!(matches!(
            VerticalFrame::parse("middle"),
            Err(AppError::InvalidVerticalFrame(_))
        ));
    }

    #[test]
    fn derive_output_path_uses_size_template_when_omitted() {
        let input = PathBuf::from("/tmp/photo.jpg");
        let output = derive_output_path(
            &input,
            Size {
                width: 800,
                height: 600,
            },
            None,
        )
        .expect("output path should be derived");

        assert_eq!(output, PathBuf::from("/tmp/photo.800x600.jpg"));
    }

    #[test]
    fn derive_output_path_appends_input_extension_for_extensionless_output() {
        let input = PathBuf::from("/tmp/photo.png");
        let explicit = PathBuf::from("/tmp/out/cropped");

        let output = derive_output_path(
            &input,
            Size {
                width: 320,
                height: 240,
            },
            Some(&explicit),
        )
        .expect("extension should be appended");

        assert_eq!(output, PathBuf::from("/tmp/out/cropped.png"));
    }

    #[test]
    fn resolve_input_path_with_single_candidate_succeeds() {
        let temp_dir = temp_dir("single_candidate");
        let image_path = temp_dir.join("image.png");
        write_color_image(&image_path, 2, 2, Rgba([255, 0, 0, 255]));

        let resolved =
            resolve_input_path(None, &temp_dir).expect("single candidate should resolve");
        assert_eq!(resolved, image_path);

        cleanup_dir(&temp_dir);
    }

    #[test]
    fn resolve_input_path_with_multiple_candidates_is_ambiguous() {
        let temp_dir = temp_dir("ambiguous_candidate");
        write_color_image(&temp_dir.join("one.png"), 2, 2, Rgba([255, 0, 0, 255]));
        write_color_image(&temp_dir.join("two.jpg"), 2, 2, Rgba([0, 255, 0, 255]));

        let result = resolve_input_path(None, &temp_dir);
        assert!(matches!(result, Err(AppError::AmbiguousInputPath { .. })));

        cleanup_dir(&temp_dir);
    }

    #[test]
    fn ensure_distinct_paths_rejects_same_file() {
        let temp_dir = temp_dir("same_paths");
        let input = temp_dir.join("image.png");
        write_color_image(&input, 2, 2, Rgba([12, 34, 56, 255]));

        let result = ensure_distinct_paths(&input, &input, &temp_dir);
        assert!(matches!(result, Err(AppError::SameInputOutput { .. })));

        cleanup_dir(&temp_dir);
    }

    #[test]
    fn crop_image_center_frame_is_deterministic() {
        let temp_dir = temp_dir("center_crop");
        let input = temp_dir.join("source.png");
        let output = temp_dir.join("result.png");

        write_horizontal_band_pattern(&input);

        let request = CropRequest {
            size: Size {
                width: 2,
                height: 2,
            },
            input_path: Some(&input),
            keep_proportion: true,
            frame_horizontal: HorizontalFrame::Center,
            frame_vertical: VerticalFrame::Center,
            output_path: Some(&output),
        };

        crop_image_in_dir(request, &temp_dir).expect("crop should succeed");

        let cropped = image::open(&output)
            .expect("output should decode")
            .to_rgba8();

        assert_eq!(cropped.dimensions(), (2, 2));
        assert_eq!(cropped.get_pixel(0, 0).0, [0, 255, 0, 255]);
        assert_eq!(cropped.get_pixel(1, 0).0, [0, 0, 255, 255]);

        cleanup_dir(&temp_dir);
    }

    #[test]
    fn crop_image_rejects_same_input_and_output_path() {
        let temp_dir = temp_dir("same_in_out");
        let input = temp_dir.join("source.png");
        write_color_image(&input, 2, 2, Rgba([10, 20, 30, 255]));

        let request = CropRequest {
            size: Size {
                width: 1,
                height: 1,
            },
            input_path: Some(&input),
            keep_proportion: true,
            frame_horizontal: HorizontalFrame::Center,
            frame_vertical: VerticalFrame::Center,
            output_path: Some(&input),
        };

        let result = crop_image_in_dir(request, &temp_dir);
        assert!(matches!(result, Err(AppError::SameInputOutput { .. })));

        cleanup_dir(&temp_dir);
    }

    #[test]
    fn crop_image_without_keep_proportion_still_hits_exact_size() {
        let temp_dir = temp_dir("no_proportion");
        let input = temp_dir.join("source.png");
        let output = temp_dir.join("result.png");

        write_color_image(&input, 4, 2, Rgba([200, 100, 20, 255]));

        let request = CropRequest {
            size: Size {
                width: 3,
                height: 3,
            },
            input_path: Some(&input),
            keep_proportion: false,
            frame_horizontal: HorizontalFrame::Left,
            frame_vertical: VerticalFrame::Top,
            output_path: Some(&output),
        };

        crop_image_in_dir(request, &temp_dir).expect("resize should succeed");
        let resized = image::open(&output).expect("output should decode");
        assert_eq!(resized.dimensions(), (3, 3));

        cleanup_dir(&temp_dir);
    }

    fn write_horizontal_band_pattern(path: &Path) {
        let mut image = RgbaImage::new(4, 2);
        for y in 0..2 {
            image.put_pixel(0, y, Rgba([255, 0, 0, 255]));
            image.put_pixel(1, y, Rgba([0, 255, 0, 255]));
            image.put_pixel(2, y, Rgba([0, 0, 255, 255]));
            image.put_pixel(3, y, Rgba([255, 255, 0, 255]));
        }

        DynamicImage::ImageRgba8(image)
            .save_with_format(path, ImageFormat::Png)
            .expect("pattern image should be written");
    }

    fn write_color_image(path: &Path, width: u32, height: u32, color: Rgba<u8>) {
        let mut image = RgbaImage::new(width, height);
        for pixel in image.pixels_mut() {
            *pixel = color;
        }

        let format =
            ImageFormat::from_path(path).expect("test path should have supported extension");
        DynamicImage::ImageRgba8(image)
            .save_with_format(path, format)
            .expect("test image should be written");
    }

    fn temp_dir(label: &str) -> PathBuf {
        let now = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .expect("clock should be after unix epoch")
            .as_nanos();
        let pid = std::process::id();
        let path = std::env::temp_dir().join(format!("imgcrop_{label}_{pid}_{now}"));

        fs::create_dir_all(&path).expect("temp dir should be created");
        path
    }

    fn cleanup_dir(path: &Path) {
        if path.exists() {
            fs::remove_dir_all(path).expect("failed to remove temp directory");
        }
    }
}
