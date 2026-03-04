use std::env;
use std::path::PathBuf;

use imgcrop::{
    crop_image, parse_size, AppError, CropRequest, HorizontalFrame, Size, VerticalFrame,
};

#[derive(Debug)]
struct CliArgs {
    size: Size,
    input_path: PathBuf,
    keep_proportion: bool,
    frame_horizontal: HorizontalFrame,
    frame_vertical: VerticalFrame,
    output_path: Option<PathBuf>,
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
        Err(ParseError::Help(help_text)) => {
            println!("{help_text}");
        }
        Err(ParseError::Message(message)) => {
            eprintln!("Error: {message}");
            eprintln!("{}", usage("imgcrop"));
            std::process::exit(1);
        }
    }
}

fn execute(args: CliArgs) -> Result<(), AppError> {
    let output_path = crop_image(CropRequest {
        size: args.size,
        input_path: Some(args.input_path.as_path()),
        keep_proportion: args.keep_proportion,
        frame_horizontal: args.frame_horizontal,
        frame_vertical: args.frame_vertical,
        output_path: args.output_path.as_deref(),
    })?;

    println!("Cropped image written to {}", output_path.display());
    Ok(())
}

fn parse_args<I>(args: I) -> Result<CliArgs, ParseError>
where
    I: IntoIterator<Item = String>,
{
    let mut args = args.into_iter();
    let executable = args.next().unwrap_or_else(|| "imgcrop".to_string());

    let input_raw = args.next().ok_or_else(|| {
        ParseError::Message(
            "missing required <PATH>. usage: imgcrop <PATH> --size <WIDTH>x<HEIGHT>".to_string(),
        )
    })?;

    if matches!(input_raw.as_str(), "-h" | "--help") {
        return Err(ParseError::Help(usage(&executable)));
    }

    if input_raw.starts_with('-') {
        return Err(ParseError::Message(format!(
            "expected <PATH> as first argument, found option: {input_raw}"
        )));
    }

    let input_path = PathBuf::from(input_raw);
    let mut size = None;
    let mut keep_proportion = true;
    let mut keep_proportion_set = false;
    let mut frame_horizontal = HorizontalFrame::Center;
    let mut frame_horizontal_set = false;
    let mut frame_vertical = VerticalFrame::Center;
    let mut frame_vertical_set = false;
    let mut output_path = None;

    while let Some(arg) = args.next() {
        match arg.as_str() {
            "-h" | "--help" => return Err(ParseError::Help(usage(&executable))),
            "--size" => {
                if size.is_some() {
                    return Err(ParseError::Message(
                        "--size provided more than once".to_string(),
                    ));
                }

                let value = next_value(&mut args, "--size")?;
                let parsed_size =
                    parse_size(&value).map_err(|error| ParseError::Message(error.to_string()))?;
                size = Some(parsed_size);
            }
            "--keep-proportion" => {
                if keep_proportion_set {
                    return Err(ParseError::Message(
                        "--keep-proportion provided more than once".to_string(),
                    ));
                }

                let value = next_value(&mut args, "--keep-proportion")?;
                keep_proportion = parse_bool(&value)?;
                keep_proportion_set = true;
            }
            "--frame-horizontal" => {
                if frame_horizontal_set {
                    return Err(ParseError::Message(
                        "--frame-horizontal provided more than once".to_string(),
                    ));
                }

                let value = next_value(&mut args, "--frame-horizontal")?;
                frame_horizontal = HorizontalFrame::parse(&value)
                    .map_err(|error| ParseError::Message(error.to_string()))?;
                frame_horizontal_set = true;
            }
            "--frame-vertical" => {
                if frame_vertical_set {
                    return Err(ParseError::Message(
                        "--frame-vertical provided more than once".to_string(),
                    ));
                }

                let value = next_value(&mut args, "--frame-vertical")?;
                frame_vertical = VerticalFrame::parse(&value)
                    .map_err(|error| ParseError::Message(error.to_string()))?;
                frame_vertical_set = true;
            }
            "--output" => {
                if output_path.is_some() {
                    return Err(ParseError::Message(
                        "--output provided more than once".to_string(),
                    ));
                }

                let value = next_value(&mut args, "--output")?;
                output_path = Some(PathBuf::from(value));
            }
            _ if arg.starts_with('-') => {
                return Err(ParseError::Message(format!("unknown option: {arg}")));
            }
            _ => {
                return Err(ParseError::Message(format!(
                    "unexpected positional argument: {arg}. expected options after <PATH>"
                )));
            }
        }
    }

    let size = size.ok_or_else(|| {
        ParseError::Message("missing required --size <WIDTH>x<HEIGHT>".to_string())
    })?;

    Ok(CliArgs {
        size,
        input_path,
        keep_proportion,
        frame_horizontal,
        frame_vertical,
        output_path,
    })
}

fn next_value<I>(args: &mut I, option: &str) -> Result<String, ParseError>
where
    I: Iterator<Item = String>,
{
    args.next()
        .ok_or_else(|| ParseError::Message(format!("missing value for {option}")))
}

fn parse_bool(raw: &str) -> Result<bool, ParseError> {
    match raw.trim().to_ascii_lowercase().as_str() {
        "true" => Ok(true),
        "false" => Ok(false),
        _ => Err(ParseError::Message(format!(
            "invalid value for --keep-proportion '{raw}'. expected true or false"
        ))),
    }
}

fn usage(executable: &str) -> String {
    format!(
        "Usage: {executable} <PATH> --size <WIDTH>x<HEIGHT> [--keep-proportion <true|false>] [--frame-horizontal <left|center|right>] [--frame-vertical <top|center|bottom>] [--output <PATH>]\n\n\
Crops an image to exact dimensions with deterministic output naming.\n\n\
Defaults:\n\
  --keep-proportion true\n\
  --frame-horizontal center\n\
  --frame-vertical center\n\n\
Output naming when --output is omitted:\n\
  - Uses the input directory and template: {{name}}.{{width}}x{{height}}.{{extension}}\n\
  - Example: photo.jpg with --size 800x600 => photo.800x600.jpg\n\n\
Examples:\n\
  {executable} ./photo.jpg --size 800x600\n\
  {executable} ./photo.jpg --size 1200x800 --frame-horizontal right\n\
  {executable} ./wide.png --size 300x300 --keep-proportion false --output ./out/square.png\n\n\
Options:\n\
  -h, --help  Show this message"
    )
}

#[cfg(test)]
mod tests {
    use super::{parse_args, ParseError};
    use imgcrop::{HorizontalFrame, VerticalFrame};

    #[test]
    fn parse_args_accepts_defaults() {
        let args = vec![
            "imgcrop".to_string(),
            "input.jpg".to_string(),
            "--size".to_string(),
            "800x600".to_string(),
        ];

        let parsed = parse_args(args).expect("args should parse");

        assert_eq!(parsed.size.width, 800);
        assert_eq!(parsed.size.height, 600);
        assert_eq!(parsed.input_path.to_string_lossy(), "input.jpg");
        assert!(parsed.keep_proportion);
        assert_eq!(parsed.frame_horizontal, HorizontalFrame::Center);
        assert_eq!(parsed.frame_vertical, VerticalFrame::Center);
        assert!(parsed.output_path.is_none());
    }

    #[test]
    fn parse_args_accepts_all_supported_options() {
        let args = vec![
            "imgcrop".to_string(),
            "input.jpg".to_string(),
            "--size".to_string(),
            "1024x768".to_string(),
            "--keep-proportion".to_string(),
            "false".to_string(),
            "--frame-horizontal".to_string(),
            "right".to_string(),
            "--frame-vertical".to_string(),
            "bottom".to_string(),
            "--output".to_string(),
            "out/result.png".to_string(),
        ];

        let parsed = parse_args(args).expect("args should parse");

        assert_eq!(parsed.size.width, 1024);
        assert_eq!(parsed.size.height, 768);
        assert_eq!(parsed.input_path.to_string_lossy(), "input.jpg");
        assert!(!parsed.keep_proportion);
        assert_eq!(parsed.frame_horizontal, HorizontalFrame::Right);
        assert_eq!(parsed.frame_vertical, VerticalFrame::Bottom);
        assert_eq!(
            parsed
                .output_path
                .expect("output should be present")
                .to_string_lossy(),
            "out/result.png"
        );
    }

    #[test]
    fn parse_args_requires_size() {
        let args = vec!["imgcrop".to_string(), "a.png".to_string()];
        let parsed = parse_args(args);
        assert!(matches!(parsed, Err(ParseError::Message(_))));
    }

    #[test]
    fn parse_args_requires_path_first() {
        let args = vec![
            "imgcrop".to_string(),
            "--size".to_string(),
            "800x600".to_string(),
        ];
        let parsed = parse_args(args);
        assert!(matches!(parsed, Err(ParseError::Message(_))));
    }

    #[test]
    fn parse_args_rejects_invalid_keep_proportion() {
        let args = vec![
            "imgcrop".to_string(),
            "--size".to_string(),
            "800x600".to_string(),
            "--keep-proportion".to_string(),
            "yes".to_string(),
        ];

        let parsed = parse_args(args);
        assert!(matches!(parsed, Err(ParseError::Message(_))));
    }

    #[test]
    fn parse_args_rejects_invalid_frame_values() {
        let args = vec![
            "imgcrop".to_string(),
            "--size".to_string(),
            "800x600".to_string(),
            "--frame-horizontal".to_string(),
            "middle".to_string(),
        ];

        let parsed = parse_args(args);
        assert!(matches!(parsed, Err(ParseError::Message(_))));
    }

    #[test]
    fn parse_args_rejects_unknown_option() {
        let args = vec![
            "imgcrop".to_string(),
            "input.jpg".to_string(),
            "--size".to_string(),
            "800x600".to_string(),
            "--unknown".to_string(),
        ];

        let parsed = parse_args(args);
        assert!(matches!(parsed, Err(ParseError::Message(_))));
    }

    #[test]
    fn parse_args_returns_help() {
        let args = vec!["imgcrop".to_string(), "--help".to_string()];
        let parsed = parse_args(args);
        assert!(matches!(parsed, Err(ParseError::Help(_))));
    }

    #[test]
    fn parse_args_rejects_extra_positional_argument() {
        let args = vec![
            "imgcrop".to_string(),
            "input.jpg".to_string(),
            "other.jpg".to_string(),
            "--size".to_string(),
            "800x600".to_string(),
        ];

        let parsed = parse_args(args);
        assert!(matches!(parsed, Err(ParseError::Message(_))));
    }
}
