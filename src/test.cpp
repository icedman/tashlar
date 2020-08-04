#include "document.h"

#include <iostream>

void run(document_t& doc)
{
    std::string txt;

    block_ptr block = doc.firstBlock();
    while (block) {
        // //block->print();
        txt = block->text() + "?!!";
        block->setText(txt);
        block = block->next();
    }

    block = doc.blockAtLine(4);
    txt = block->text() + " modified at 4";
    block->setText("txt");

    for (int i = 0; i < 4; i++) {
        block_ptr block = doc.addBlockAtLine(0);
        txt = "added at 0";
        block->setText(txt);
    }

    for (int i = 0; i < 4; i++) {
        block_ptr block = doc.addBlockAtLine(20);
        txt = "added at ";
        txt += std::to_string(20);
        block->setText(txt);
    }

    block = doc.firstBlock();
    while (block) {
        block->print();
        block = block->next();
    }

    cursor_t cursor = doc.cursor();
    block = cursor.block();
    block->setText("hello");

    doc.print();
}

int main()
{
    document_t doc;
    doc.open("./tests/test.cpp");

    cursor_t cursor = doc.cursor();

    /*
    cursor.moveRight(19);
    cursor.eraseText(20);
    */

    /*
    cursor.moveRight(1);
    cursor.moveDown(3);
    cursor.insertText("hello");
    cursor.moveUp(2);
    cursor.insertText("world");
    */

    /*
    cursor.moveRight(51);
    // cursor.moveRight(0);
    cursor.insertText("hello");
    cursor.moveLeft(13);
    cursor.insertText("world");
    */

    //cursor.moveRight(1);
    //cursor.moveRight(2, true);
    // cursor.moveRight(8);
    // cursor.moveDown(1, true);
    //cursor.moveStartOfLine(true);
    //cursor.moveRight(8, true);
    //std::cout << "selected:" << cursor.selectedText() << std::endl;

    // cursor.moveRight(4);
    // cursor.splitLine();
    // cursor.mergeNextLine();

    // cursor.eraseSelection();
    // doc.removeBlockAtLine(0);

    cursor.moveRight();
    cursor.moveDown();
    doc.addCursor(cursor);
    cursor.moveUp();
    cursor.moveRight(7);
    doc.addCursor(cursor);
    cursor.moveLeft(7);
    doc.setCursor(cursor);

    cursor_list cursors = doc.cursors;
    cursor_util::sortCursors(cursors);

    for (auto& c : cursors) {
        c.print();
    }

    for (auto& c : cursors) {
        c.splitLine();
        c.moveDown();
        c.setPosition(c.block(), 0);
        c.insertText("hello");
        // cursor_util::advanceBlockCursors(cursors, c, 5);
    }

    for (auto& c : cursors) {
        // c.moveLeft(2);
        // c.eraseText(2);
        // cursor_util::advanceBlockCursors(cursors, c, -2);
    }
    doc.updateCursors(cursors);

    cursor = doc.cursor();
    cursor.moveDown();
    cursor.insertText("?");
    doc.print();
    return 0;
}