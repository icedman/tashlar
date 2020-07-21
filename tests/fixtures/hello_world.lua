command("open", "tests/source/test.cpp")

command("move_cursor_end_of_document")
command("enter");
command("insert", "end of document")

command("move_cursor_start_of_document")
command("insert", "start of document\nhello world\n")

command("move_cursor_down");
command("move_cursor_down");
command("move_cursor_down");
command("move_cursor_down");
command("move_cursor_down");

command("indent");
command("insert", "five cursors down")
command("enter");

command("save_as", "tests/results/hello_word.cpp")

