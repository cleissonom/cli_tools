use std::env;
use std::path::PathBuf;

use imgconvert::{convert_image, AppError};

#[derive(Debug)]
struct CliArgs {
    input_path: PathBuf,
    target_extension: String,
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
            eprintln!("{}", usage("imgconvert"));
            std::process::exit(1);
        }
    }
}

fn execute(args: CliArgs) -> Result<(), AppError> {
    let output_path = convert_image(
        &args.input_path,
        &args.target_extension,
        args.output_path.as_deref(),
    )?;

    println!("Converted image written to {}", output_path.display());
    Ok(())
}

fn parse_args<I>(args: I) -> Result<CliArgs, ParseError>
where
    I: IntoIterator<Item = String>,
{
    let mut args = args.into_iter();
    let executable = args.next().unwrap_or_else(|| "imgconvert".to_string());

    let mut positionals = Vec::new();
    for arg in args {
        match arg.as_str() {
            "-h" | "--help" => return Err(ParseError::Help(usage(&executable))),
            _ if arg.starts_with('-') => {
                return Err(ParseError::Message(format!("unknown option: {arg}")));
            }
            _ => positionals.push(arg),
        }
    }

    if positionals.len() < 2 {
        return Err(ParseError::Message(
            "expected at least <input_image> and <new_extension>".to_string(),
        ));
    }

    if positionals.len() > 3 {
        return Err(ParseError::Message(
            "too many arguments. expected: <input_image> <new_extension> [output_image]"
                .to_string(),
        ));
    }

    let input_path = PathBuf::from(positionals[0].clone());
    let target_extension = positionals[1].clone();
    let output_path = positionals.get(2).map(PathBuf::from);

    Ok(CliArgs {
        input_path,
        target_extension,
        output_path,
    })
}

fn usage(executable: &str) -> String {
    format!(
        "Usage: {executable} <input_image> <new_extension> [output_image]\n\n\
Converts an image to the requested extension.\n\
If [output_image] is omitted, a sibling file is created with the same base name and the new extension.\n\n\
Examples:\n\
  {executable} ./photo.jpg png\n\
  {executable} ./photo.jpg webp ./converted/new-name.webp\n\
  {executable} ./photo.png jpg ./converted/photo_copy\n\n\
Options:\n\
  -h, --help  Show this message"
    )
}

#[cfg(test)]
mod tests {
    use super::{parse_args, ParseError};

    #[test]
    fn parse_args_accepts_optional_output_path() {
        let args = vec![
            "imgconvert".to_string(),
            "input.jpg".to_string(),
            "png".to_string(),
            "out/image_name.png".to_string(),
        ];

        let parsed = parse_args(args).expect("args should parse");

        assert_eq!(parsed.input_path.to_string_lossy(), "input.jpg");
        assert_eq!(parsed.target_extension, "png");
        assert_eq!(
            parsed
                .output_path
                .expect("output path should exist")
                .to_string_lossy(),
            "out/image_name.png"
        );
    }

    #[test]
    fn parse_args_requires_input_and_extension() {
        let args = vec!["imgconvert".to_string(), "input.jpg".to_string()];
        let parsed = parse_args(args);

        assert!(matches!(parsed, Err(ParseError::Message(_))));
    }

    #[test]
    fn parse_args_rejects_unknown_option() {
        let args = vec![
            "imgconvert".to_string(),
            "--unknown".to_string(),
            "input.jpg".to_string(),
            "png".to_string(),
        ];

        let parsed = parse_args(args);
        assert!(matches!(parsed, Err(ParseError::Message(_))));
    }

    #[test]
    fn parse_args_rejects_extra_positionals() {
        let args = vec![
            "imgconvert".to_string(),
            "input.jpg".to_string(),
            "png".to_string(),
            "output.png".to_string(),
            "unexpected.txt".to_string(),
        ];

        let parsed = parse_args(args);
        assert!(matches!(parsed, Err(ParseError::Message(_))));
    }
}
