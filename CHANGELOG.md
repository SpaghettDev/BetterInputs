# BetterInputs Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).

## [v4.2.0-beta.1] - 2024-11-17

### Changed

- Target Geode version (v3.3.0 -> v4.0.0-beta.1)

## [v4.2.0-beta.1] - 2024-07-22

### Changed

- Target Geode version (v3.0.0-beta.2 -> v3.3.0)

### Fixed

- Incompatibility with Enter Search mod (#5) - Thanks @iswilljr

## [v4.1.0-beta] - 2024-06-19

### Added

- Ability to bypass max input length (can be disabled)

### Changed

- How the AlertFix works, again... Checks for touch priority if the layer has a touch priority, if not then it falls back to Z order (it was also causing a crash in `LevelEditLayer`)

### Fixed

- Crash in level editor
- Pasting text bypassing allowed characters filter
- Space character weirdness in `CCTextInputNode`s with `CCLabelBMFont`s
- More crashes! (when highlighting text)

## [v4.0.0-beta] - 2024-06-13

### Added

- Some more keybinds! (Alphalaneous wanted them)
  - Ctrl+Del, Ctrl+Backspace
  - Home/End, Shift+Home, Shift+End
  - Ctrl+Ins
  - Shift+Ins
- A TextArea cursor fix. All it does is fix a weird vanilla bug where the cursor is positioned sometimes in the middle of the character
- Actual MacOS support (still broken, binary included but will do nothing, in fact it will break input nodes :trollface:)

### Changed

- How the AlertLayer fix works (it checks for touch priority instead of Z order now)
- How `insertCharAtIndex` and `insertStrAtIndex` to be more efficient (it resizes the string if needed by double, instead of always resizing)

### Fixed

- Numerous text selection bugs in `TextArea`s
- Ctrl+Left/Right arrow not positioning cursor correctly (skipping to the character before/after the separator)
- A lot of other bugs

## [v3.1.0-beta] - 2024-06-08

### Fixed

- Dragging input in the index crashing the game
- Some mouse issues

## [v3.0.0-beta] - 2024-06-06

### Added

- MacOS support :o

### Changed

- Target Geode version (v2.0.0-beta.27 -> v3.0.0-alpha.1)

## [2.2.1-beta] - 2024-06-01

### Changed

- For loop in layer detection to just use last node

## [2.2.0-beta] - 2024-06-01

### Added

- `Auto deselect input` option, default is enabled. If on, and when ESC is clicked while an input is selected, closes the alert/layer that is in front of the input instead of deselecting the input.

### Fixed

- `CCTextInputNode` swallowing ESC key of Alerts/Layers above it
- `m_string` not getting updated when `CCTextInputNode::setString` is called

## [2.1.0-beta] - 2024-06-01

Robert where update 👀

### Added

- CharNode struct, returned when calling `getCharNodePosInfo` or `getCharNodePosInfoAtLine`

### Fixed

- Text being inserted somewhere completely wrong in `TextArea`s when highlighting
- Cursor also being positioned somewhere wrong in `TextArea`s when highlighting

### Changed

- Moved helper types into `/src/types` folder

## [2.0.0-beta] - 2024-05-31

### Fixed

- Everything (`TextArea` selection bugs, `TextArea` highlight sprite going everywhere, and much, much more)

## [1.4.0] - 2024-05-30

### Fixed

- Cursor position funkiness in `TextArea`s and regular `CCTextInputNode`s, again
- Cursor position not getting set when using mouse in `TextArea`s

## [1.3.2] - 2024-05-28

### Fixed

- Cursor position funkiness in `TextArea`s and regular `CCTextInputNode`s

## [1.3.1] - 2024-05-27

### Fixed

- More `TextArea` selection bugs

## [1.3.0] - 2024-05-27

### Fixed

- `TextArea` selection

## [1.2.0] - 2024-05-26

### Fixed

- Text selection bugs

## [1.1.0] - 2024-05-25

### Fixed

- Text selection and copy/paste shortcuts

## [1.0.1] - 2024-05-26

### Changed

- Logo
- Target geode version (v2.0.0-beta.26 -> v2.0.0-beta.27)

## [1.0.0] - 2024-05-25

### Added

- The project
