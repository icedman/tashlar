app.command("open", "tests/source/test.cpp")

app.command("move_cursor_end_of_document")
app.command("enter");
app.command("insert", "end of document")

app.command("move_cursor_start_of_document")
app.command("insert", "start of document\nhello world\n")

app.command("move_cursor_down");
app.command("move_cursor_down");
app.command("move_cursor_down");
app.command("move_cursor_down");
app.command("move_cursor_down");

app.command("indent");
app.command("insert", "five cursors down")
app.command("enter");

app.command("save_as", "tests/results/hello_word.cpp")

