#include <eterm/terminal/terminalemulator.hpp>
#include <eterm/terminal/iterminaldisplay.hpp>
#include <eterm/terminal/ipseudoterminal.hpp>
#include <eterm/system/iprocess.hpp>
#include "utest.hpp"

using namespace eterm::Terminal;
using namespace eterm::System;

class MockPty : public IPseudoTerminal {
public:
    std::string mBuffer;
    int mCols = 80;
    int mRows = 24;
    int getNumColumns() const override { return mCols; }
    int getNumRows() const override { return mRows; }
    bool resize(int columns, int rows) override { mCols = columns; mRows = rows; return true; }
    bool isTTY() const override { return true; }
    int write(const char* s, size_t n) override {
        mBuffer.append(s, n);
        return n;
    }
    int read(char* buf, size_t n, bool) override {
        if (mBuffer.empty()) return 0;
        size_t toRead = std::min(n, mBuffer.size());
        memcpy(buf, mBuffer.data(), toRead);
        mBuffer.erase(0, toRead);
        return toRead;
    }
};

class MockProcess : public IProcess {
public:
    void checkExitStatus() override {}
    bool hasExited() const override { return false; }
    int getExitCode() const override { return 0; }
    void terminate() override {}
    void waitForExit() override {}
    int pid() override { return 123; }
};

class MockDisplay : public ITerminalDisplay {
public:
    bool drawBegin(Uint32, Uint32) override { return true; }
    void drawLine(Line, int, int, int) override {}
    void drawCursor(int, int, TerminalGlyph, int, int, TerminalGlyph) override {}
    void drawEnd() override {}
};

UTEST(eterm, basic_write) {
    auto pty = std::make_unique<MockPty>();
    auto process = std::make_unique<MockProcess>();
    auto display = std::make_shared<MockDisplay>();
    auto term = TerminalEmulator::create(std::move(pty), std::move(process), display, 100);

    term->write("ABC", 3);
    term->update();
    
    term->selstart(0, 0, 0);
    term->selextend(2, 0, 1, 0);
    EXPECT_TRUE(term->hasSelection());
    EXPECT_STDSTREQ("ABC", term->getSelection());
}

UTEST(eterm, selection_reflow) {
    auto pty = std::make_unique<MockPty>();
    auto process = std::make_unique<MockProcess>();
    auto display = std::make_shared<MockDisplay>();
    auto term = TerminalEmulator::create(std::move(pty), std::move(process), display, 100);

    // 80x24. Write 80 'A's then 80 'B's.
    std::string row0(80, 'A');
    std::string row1(80, 'B');
    term->write(row0.c_str(), row0.size());
    term->write(row1.c_str(), row1.size());
    term->write(" ", 1); // Trigger wrap on row 1 to move cursor to row 2 and preserve row 0 wrap
    term->update();
    
    // Selection from index 70 of row 0 to index 10 of row 1.
    term->selstart(70, 0, 0); 
    term->selextend(10, 1, 1, 0);
    
    // ATTR_WRAP is set on row 0, so no newline should be added between A and B.
    std::string expected = std::string(10, 'A') + std::string(11, 'B');
    std::string sel = term->getSelection();
    EXPECT_STDSTREQ(expected, sel);
    
    // Resize to 40 columns
    term->resize(40, 24);
    
    EXPECT_TRUE(term->hasSelection());
    EXPECT_STDSTREQ(expected, term->getSelection());
}

UTEST(eterm, selection_reflow_history) {
    auto pty = std::make_unique<MockPty>();
    auto process = std::make_unique<MockProcess>();
    auto display = std::make_shared<MockDisplay>();
    auto term = TerminalEmulator::create(std::move(pty), std::move(process), display, 100);

    // Fill history with unique lines, each 40 chars to ensure they fit in 80.
    for (int i = 0; i < 40; ++i) {
        std::string line = "H" + std::to_string(i) + " " + std::string(30, 'x') + "\n";
        term->write(line.c_str(), line.size());
        term->update();
    }
    
    // 40 lines total. 24 on screen. 16 in history.
    // Let's select Line 30 (which is on screen)
    // Row 0 is Line 16. Row 14 is Line 30.
    term->selstart(0, 14, 0);
    term->selextend(1, 14, 1, 0);
    
    std::string sel = term->getSelection();
    EXPECT_FALSE(sel.empty());
    
    term->resize(40, 24);
    
    EXPECT_TRUE(term->hasSelection());
    EXPECT_STDSTREQ(sel, term->getSelection());
}

UTEST(eterm, selection_rectangular) {
    auto pty = std::make_unique<MockPty>();
    auto process = std::make_unique<MockProcess>();
    auto display = std::make_shared<MockDisplay>();
    auto term = TerminalEmulator::create(std::move(pty), std::move(process), display, 100);

    term->write("Line 1: ABCDEFG\r\n", 17);
    term->write("Line 2: HIJKLMN\r\n", 17);
    term->write("Line 3: OPQRSTU\r\n", 17);
    term->update();

    // Select "ABC", "HIJ", "OPQ" area
    // "Line 1: " is 8 chars. A is at col 8.
    term->selstart(8, 0, 0);
    term->selextend(10, 2, 2, 0); // Type 2 = SEL_RECTANGULAR

    std::string sel = term->getSelection();
    EXPECT_STDSTREQ("ABC\nHIJ\nOPQ", sel);
}

UTEST(eterm, selection_reverse) {
    auto pty = std::make_unique<MockPty>();
    auto process = std::make_unique<MockProcess>();
    auto display = std::make_shared<MockDisplay>();
    auto term = TerminalEmulator::create(std::move(pty), std::move(process), display, 100);

    term->write("Line 1\r\nLine 2\r\nLine 3", 22);
    term->update();

    // Select from Line 3 to Line 1
    term->selstart(5, 2, 0);
    term->selextend(0, 0, 1, 0);

    EXPECT_STDSTREQ("Line 1\nLine 2\nLine 3", term->getSelection());
}

UTEST(eterm, selection_wrap) {
    auto pty = std::make_unique<MockPty>();
    auto process = std::make_unique<MockProcess>();
    auto display = std::make_shared<MockDisplay>();
    auto term = TerminalEmulator::create(std::move(pty), std::move(process), display, 100);

    // Terminal is 80x24.
    std::string longLine(80, 'A');
    longLine += "BBBB";
    term->write(longLine.c_str(), longLine.size());
    term->update();

    // Selection should not have a newline at the wrap point
    term->selstart(78, 0, 0);
    term->selextend(2, 1, 1, 0);

    std::string sel = term->getSelection();
    EXPECT_STDSTREQ("AABBB", sel);
}

UTEST(eterm, selection_snap_word) {
    auto pty = std::make_unique<MockPty>();
    auto process = std::make_unique<MockProcess>();
    auto display = std::make_shared<MockDisplay>();
    auto term = TerminalEmulator::create(std::move(pty), std::move(process), display, 100);

    term->write("Hello World Test", 16);
    term->update();

    // Snap to "World"
    // World starts at index 6
    term->selstart(7, 0, 1); // Type 1 = SNAP_WORD
    term->selextend(7, 0, 1, 0);

    EXPECT_STDSTREQ("World", term->getSelection());
}

UTEST(eterm, selection_snap_line) {
    auto pty = std::make_unique<MockPty>();
    auto process = std::make_unique<MockProcess>();
    auto display = std::make_shared<MockDisplay>();
    auto term = TerminalEmulator::create(std::move(pty), std::move(process), display, 100);

    term->write("Line 1\r\nLine 2\r\nLine 3", 22);
    term->update();

    // Snap to Line 2
    term->selstart(2, 1, 2); // Type 2 = SNAP_LINE
    term->selextend(2, 1, 1, 0);

    EXPECT_STDSTREQ("Line 2\n", term->getSelection());
}

UTEST(eterm, selection_alt_screen) {
    auto pty = std::make_unique<MockPty>();
    auto process = std::make_unique<MockProcess>();
    auto display = std::make_shared<MockDisplay>();
    auto term = TerminalEmulator::create(std::move(pty), std::move(process), display, 100);

    term->write("Main Screen", 11);
    term->update();

    // Switch to alt screen and reset cursor position to (0,0)
    term->write("\033[?1049h\033[H", 11);
    term->update();
    
    term->write("Alt Screen", 10);
    term->update();

    term->selstart(0, 0, 0);
    term->selextend(2, 0, 1, 0);
    EXPECT_STDSTREQ("Alt", term->getSelection());

    // Switch back to main
    term->write("\033[?1049l", 8);
    term->update();

    // Selection should be cleared or at least not "Alt"
    EXPECT_FALSE(term->hasSelection());
}

UTEST(eterm, selection_scrolling) {
    auto pty = std::make_unique<MockPty>();
    auto process = std::make_unique<MockProcess>();
    auto display = std::make_shared<MockDisplay>();
    auto term = TerminalEmulator::create(std::move(pty), std::move(process), display, 100);

    term->write("Target Line\r\n", 13);
    term->update();

    // Select "Target"
    term->selstart(0, 0, 0);
    term->selextend(5, 0, 1, 0);
    EXPECT_STDSTREQ("Target", term->getSelection());

    // Push it into history by writing 30 lines
    for (int i = 0; i < 30; ++i) {
        term->write("New Line\r\n", 10);
    }
    term->update();

    // Selection should have moved with the text
    EXPECT_TRUE(term->hasSelection());
    EXPECT_STDSTREQ("Target", term->getSelection());
}

UTEST(eterm, selection_tabs) {
    auto pty = std::make_unique<MockPty>();
    auto process = std::make_unique<MockProcess>();
    auto display = std::make_shared<MockDisplay>();
    auto term = TerminalEmulator::create(std::move(pty), std::move(process), display, 100);

    // Default tab stop is 4
    term->write("A\tB", 3);
    term->update();

    // Select A[tab]B
    // A is at 0, tab is at 1,2,3, B is at 4
    term->selstart(0, 0, 0);
    term->selextend(4, 0, 1, 0);

    std::string sel = term->getSelection();
    EXPECT_STDSTREQ("A   B", sel);
}

UTEST(eterm, selection_unicode) {
    auto pty = std::make_unique<MockPty>();
    auto process = std::make_unique<MockProcess>();
    auto display = std::make_shared<MockDisplay>();
    auto term = TerminalEmulator::create(std::move(pty), std::move(process), display, 100);

    // Write some UTF-8 text: "Héllo Wörld"
    // é is C3 A9, ö is C3 B6
    term->write("H\xC3\xA9llo W\xC3\xB6rld", 13);
    term->update();

    term->selstart(0, 0, 0);
    term->selextend(10, 0, 1, 0); // Select "Héllo Wörld"

    EXPECT_STDSTREQ("Héllo Wörld", term->getSelection());
}

UTEST(eterm, selection_wide_chars) {
    auto pty = std::make_unique<MockPty>();
    auto process = std::make_unique<MockProcess>();
    auto display = std::make_shared<MockDisplay>();
    auto term = TerminalEmulator::create(std::move(pty), std::move(process), display, 100);

    // Unicode Emoji is often wide (2 columns)
    // Rocket 🚀 is F0 9F 9A 80
    term->write("A\xF0\x9F\x9A\x80Z", 6);
    term->update();

    // A is at 0, 🚀 is at 1-2, Z is at 3
    term->selstart(0, 0, 0);
    term->selextend(3, 0, 1, 0);

    EXPECT_STDSTREQ("A🚀Z", term->getSelection());

    // Test selection starting/ending in the middle of a wide char
    term->selstart(1, 0, 0); // Start at first half of rocket
    term->selextend(2, 0, 1, 0); // End at second half
    EXPECT_STDSTREQ("🚀", term->getSelection());
    
    term->selstart(1, 0, 0);
    term->selextend(1, 0, 1, 0);
    EXPECT_STDSTREQ("🚀", term->getSelection());
}

UTEST(eterm, selection_reflow_extreme) {
    auto pty = std::make_unique<MockPty>();
    auto process = std::make_unique<MockProcess>();
    auto display = std::make_shared<MockDisplay>();
    auto term = TerminalEmulator::create(std::move(pty), std::move(process), display, 100);

    // Initial 80x24. Write a long line.
    std::string text = "A VERY LONG LINE THAT WILL BE REFLOWED TO A NARROW TERMINAL";
    term->write(text.c_str(), text.size());
    term->update();

    // Select "REFLOWED"
    // text[30] to text[37]
    term->selstart(30, 0, 0);
    term->selextend(37, 0, 1, 0);
    EXPECT_STDSTREQ("REFLOWED", term->getSelection());

    // Shrink to 5 columns
    term->resize(5, 24);
    
    // REFLOWED should still be selected
    EXPECT_TRUE(term->hasSelection());
    EXPECT_STDSTREQ("REFLOWED", term->getSelection());
    
    // Expand back to 80 columns
    term->resize(80, 24);
    EXPECT_STDSTREQ("REFLOWED", term->getSelection());
}

UTEST(eterm, selection_clear_screen) {
    auto pty = std::make_unique<MockPty>();
    auto process = std::make_unique<MockProcess>();
    auto display = std::make_shared<MockDisplay>();
    auto term = TerminalEmulator::create(std::move(pty), std::move(process), display, 100);

    term->write("Test text", 9);
    term->update();

    term->selstart(0, 0, 0);
    term->selextend(3, 0, 1, 0);
    EXPECT_TRUE(term->hasSelection());

    // CSI 2 J - Clear Screen
    term->write("\033[2J", 4);
    term->update();

    EXPECT_FALSE(term->hasSelection());
}

UTEST(eterm, selection_scroll_region) {
    auto pty = std::make_unique<MockPty>();
    auto process = std::make_unique<MockProcess>();
    auto display = std::make_shared<MockDisplay>();
    auto term = TerminalEmulator::create(std::move(pty), std::move(process), display, 100);

    // Initial 80x24. Fill with some text.
    for (int i = 0; i < 10; ++i) {
        std::string line = "Line " + std::to_string(i) + "\r\n";
        term->write(line.c_str(), line.size());
    }
    term->update();

    // Select "Line 5" at Row 5.
    term->selstart(0, 5, 0);
    term->selextend(5, 5, 1, 0);
    EXPECT_STDSTREQ("Line 5", term->getSelection());

    // Set scrolling region: 3rd row to 8th row. (1-indexed CSI r)
    term->write("\033[3;8r", 6);
    // Move cursor to 8th row (bottom of scroll region)
    term->write("\033[8;1H", 6);
    // Write 2 more lines to push Row 5 up by 2 within the region.
    term->write("Push 1\nPush 2\n", 14);
    term->update();

    // Line 5 was at Row 5. Within [3,8], it should move to Row 3.
    // However, if it moves out of the region or something weird happens?
    // Let's check where it is.
    // Actually, tscrollup(top, n, copyhist) is used.
    // In our case top=2, bot=7. n=2.
    // Row 5 should move to 5-2 = 3.
    EXPECT_STDSTREQ("Line 5", term->getSelection());
}

UTEST(eterm, selection_trailing_spaces) {
    auto pty = std::make_unique<MockPty>();
    auto process = std::make_unique<MockProcess>();
    auto display = std::make_shared<MockDisplay>();
    auto term = TerminalEmulator::create(std::move(pty), std::move(process), display, 100);

    // Terminal is 80 columns.
    // Write "Hello   " (3 spaces) then newline.
    term->write("Hello   \r\nWorld", 15);
    term->update();

    // Select both lines.
    term->selstart(0, 0, 0);
    term->selextend(4, 1, 1, 0); // "Hello" to "World"

    // Trailing spaces on the first line should be stripped because it's not wrapped.
    EXPECT_STDSTREQ("Hello\nWorld", term->getSelection());

    // Now test with WRAPPED line.
    // Row 1 has "World" (5 chars).
    // Write 73 'A's and 2 spaces to reach 80 chars.
    std::string fill(73, 'A');
    term->write(fill.c_str(), fill.size());
    term->write("  ", 2); // Row 1 is now 80 chars: "World" + fill + "  "
    term->write("BB", 2);   // This forces a wrap. Row 2 will be "BB".
    term->update();

    // Select Row 1 and Row 2.
    // Row 1 starts at Col 0, Row 1. Row 2 starts at Col 0, Row 2.
    term->selstart(0, 1, 0);
    term->selextend(1, 2, 1, 0); // From "World" to "BB"
    
    // The spaces at the end of Row 1 should be preserved because it wrapped.
    // "World" + fill + "  " + "BB"
    std::string expected = "World" + fill + "  BB";
    EXPECT_STDSTREQ(expected, term->getSelection());
}

UTEST(eterm, selection_word_snap_unicode) {
    auto pty = std::make_unique<MockPty>();
    auto process = std::make_unique<MockProcess>();
    auto display = std::make_shared<MockDisplay>();
    auto term = TerminalEmulator::create(std::move(pty), std::move(process), display, 100);

    // Write "Héllo-Wörld"
    // Word delimiters are only space ' ' and null 0 in current implementation.
    // So "Héllo-Wörld" should be one word.
    term->write("H\xC3\xA9llo-W\xC3\xB6rld", 14);
    term->update();

    // Snap to word starting at "ll"
    term->selstart(2, 0, 1); // Index 2 is 'l'
    term->selextend(2, 0, 1, 0);

    EXPECT_STDSTREQ("Héllo-Wörld", term->getSelection());

    // Write "Test Wörld"
    term->write("\r\nTest W\xC3\xB6rld", 13);
    term->update();

    // Snap to "Wörld"
    term->selstart(6, 1, 1); // index 6 is 'W'
    term->selextend(6, 1, 1, 0);
    EXPECT_STDSTREQ("Wörld", term->getSelection());
}

UTEST(eterm, selection_history_screen_boundary) {
    auto pty = std::make_unique<MockPty>();
    auto process = std::make_unique<MockProcess>();
    auto display = std::make_shared<MockDisplay>();
    auto term = TerminalEmulator::create(std::move(pty), std::move(process), display, 100);

    term->resize(80, 5); // 5 rows terminal

    // Write 5 lines.
    for (int i = 0; i < 5; ++i) {
        std::string line = "Line" + std::to_string(i) + "\r\n";
        term->write(line.c_str(), line.size());
    }
    term->update();
    
    // Now screen has Row 0 empty, Row 1 empty... Row 4 empty?
    // Let's check. 5 rows: 0, 1, 2, 3, 4.
    // Line0\r\n -> cursor at Row 1.
    // Line1\r\n -> cursor at Row 2.
    // Line2\r\n -> cursor at Row 3.
    // Line3\r\n -> cursor at Row 4.
    // Line4\r\n -> cursor at Row 5 -> scroll up.
    // Row 0 has Line1, Row 1 has Line2, Row 2 has Line3, Row 3 has Line4.
    // Row 4 is empty. History has Line0.
    
    // Let's select Line1 (Row 0) to Line4 (Row 3).
    term->selstart(0, 0, 0);
    term->selextend(4, 3, 1, 0);
    EXPECT_TRUE(term->hasSelection());
    
    // Scroll down 2 more lines.
    term->write("New1\r\nNew2\r\n", 12);
    term->update();
    
    // Selection should have moved to history.
    // Line1 was at Row 0, moved up by 2 -> Row -2.
    // Line4 was at Row 3, moved up by 2 -> Row 1.
    EXPECT_TRUE(term->hasSelection());
    std::string sel = term->getSelection();
    EXPECT_TRUE(sel.find("Line1") != std::string::npos);
    EXPECT_TRUE(sel.find("Line4") != std::string::npos);
}

UTEST(eterm, selection_basic_history) {
    auto pty = std::make_unique<MockPty>();
    auto process = std::make_unique<MockProcess>();
    auto display = std::make_shared<MockDisplay>();
    auto term = TerminalEmulator::create(std::move(pty), std::move(process), display, 100);

    term->resize(80, 2); // 2 rows terminal: Row 0 and Row 1.

    term->write("Line0\r\n", 7);
    term->write("Line1\r\n", 7);
    term->write("Line2", 5);
    term->update();

    // Line0 is at Row -1 (history)
    // Line1 is at Row 0 (screen)
    // Line2 is at Row 1 (screen)
    
    // Select Line0 (history) and Line1 (screen).
    term->selstart(0, -1, 0);
    term->selextend(4, 0, 1, 0);
    
    EXPECT_TRUE(term->hasSelection());
    std::string sel = term->getSelection();
    EXPECT_TRUE(sel.find("Line0") != std::string::npos);
    EXPECT_TRUE(sel.find("Line1") != std::string::npos);
    EXPECT_TRUE(sel.find("Line2") == std::string::npos);
}

UTEST(eterm, selection_rectangular_reflow) {
    auto pty = std::make_unique<MockPty>();
    auto process = std::make_unique<MockProcess>();
    auto display = std::make_shared<MockDisplay>();
    auto term = TerminalEmulator::create(std::move(pty), std::move(process), display, 100);

    // Initial 80x24.
    term->write("ABCDE\r\n", 7);
    term->write("FGHIJ\r\n", 7);
    term->update();

    // Select BC and GH (Rectangular)
    // BC is at (1,0) to (2,0)
    // GH is at (1,1) to (2,1)
    term->selstart(1, 0, 0);
    term->selextend(2, 1, 2, 0); // type 2 = Rectangular
    
    EXPECT_STDSTREQ("BC\nGH", term->getSelection());

    // Resize to 5 columns.
    term->resize(5, 24);
    
    // Rectangular selections are currently cleared on resize.
    EXPECT_FALSE(term->hasSelection());
    EXPECT_STDSTREQ("", term->getSelection());
}
