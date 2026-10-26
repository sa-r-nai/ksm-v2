K-Shoot MANIA v2
========================

## ビルドに必要な環境 / Build Requirements

- Visual Studio 2022
    - Visual Studio Installer上で「C++ によるデスクトップ開発」にチェックを付けてインストールしてください /  
      Install "Desktop development with C++" from the Visual Studio Installer
- OpenSiv3D 0.6.16 (installed to Visual Studio 2022)
    - 下記からインストーラをダウンロードして実行してください / Download and run the installer below
        - https://github.com/Siv3D/OpenSiv3D#downloads

## KSON editor comment commands

The three effects below can be triggered by timed KSON editor comments.
Add a complete `command=duration` line to a comment at the desired chart position:

```json
{
  "editor": {
    "comment": [
      [0, "ui_toggle=50"],
      [960, "ui_toggle=50"],
      [1920, "lane_toggle=50\ncolor_invert=50"],
      [2880, "lane_toggle=50\ncolor_invert=50"]
    ]
  }
}
```

Positions are in KSON pulses: 240 pulses per quarter note, or 960 per 4/4 measure.
Each entry starts its commands at that position. Multiple commands may be separated by newlines in one comment, or placed in separate comments at the same position.
Spaces around the command and `=` are allowed. Ordinary memo lines and invalid durations are ignored; text containing an example such as `Example: ui_toggle=50` does not execute.
Comments remain editor comments when saved. Existing KSH body options and KSON `compat.ksh_unknown.option` commands are also supported.
A valid explicit option takes precedence over comments for the same command at the same pulse, preventing duplicate toggles.

## Effect: lane_toggle

The chart body supports USC-compatible `lane_toggle=<duration>` commands.
Each command alternates between a translucent dark-gray lane background and the original lane skin, with a wipe from the far end of the lane.
The toggled background uses dark gray (#404040) at 40% opacity, allowing the background to remain visible through the lane. Center-split lanes retain their separated shape.
Notes, measure lines, judgment effects, and scoring remain active.

The duration is measured in 1/192 of a whole note, using the BPM at the command position.
For example, `lane_toggle=50` takes approximately 0.521 seconds at 120 BPM; the next `lane_toggle=50` restores the original lane skin.
Use `lane_toggle=0` for an immediate change. Place commands before the note row where the transition should begin.
The same commands are supported in KSON charts through `compat.ksh_unknown.option.lane_toggle`.

## Effect: color_invert

Place `color_invert=<duration>` before a note row to toggle color inversion.
The inverted area expands as a circle from the exact center of the screen; the next command shrinks it back toward the center.
Backgrounds, lanes, and BT notes are inverted. FX chip notes (including key-sound chips), FX long notes, HUD, judgment indicators, laser notes, and laser cursors retain their original colors.

Timing uses the same units and event BPM as `lane_toggle`: `color_invert=50` takes approximately 0.521 seconds at 120 BPM.
Use `color_invert=0` for an immediate toggle. Both commands operate independently and support seeking and playback speed changes.
KSON charts can use `compat.ksh_unknown.option.color_invert`.

## Effect: ui_toggle

Place `ui_toggle=<duration>` before a note row to slide the HUD off screen, then use the command again to bring it back.
Song information and score slide upward, the gauge slides right, and bottom HUD elements slide downward.
The movie panel and mouse Back button slide left. Opacity is not changed by this effect.
Combo numbers, judgment feedback, achievement messages, the lane, and notes remain visible and gameplay continues.

Timing matches `lane_toggle` and `color_invert`: `ui_toggle=50` takes approximately 0.521 seconds at 120 BPM; `ui_toggle=0` changes the position immediately.
The commands are independent. Seeking, pausing, and playback speed changes use the same chart time calculation.
KSON charts can use `compat.ksh_unknown.option.ui_toggle`.

## Contributionについて

本プロジェクトは個人プロジェクトであり、オープンソース主導の開発プロジェクトではありません。  
そのため、機能実装についてのPull Requestは基本的にマージできない場合が多いです。機能追加のご提案はIssueまたはK-Shoot MANIA掲示板( https://bbs.kshootmania.com )よりお願いいたします。

既存処理の不具合修正に関するContributionについては、Pull Requestにて歓迎いたします。
