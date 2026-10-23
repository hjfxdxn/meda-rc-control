"""Regenerate the Arduino web page after editing control_panel.html."""
from pathlib import Path

folder = Path(__file__).resolve().parent
page = (folder / "control_panel.html").read_text(encoding="utf-8")
assert ')MEDAPANEL"' not in page
(folder / "control_panel_page.h").write_text(
    '#pragma once\n// Generated from control_panel.html by embed_panel.py.\n'
    'const char kControlPanelHtml[] = R"MEDAPANEL(' + page + ')MEDAPANEL";\n',
    encoding="utf-8",
)
