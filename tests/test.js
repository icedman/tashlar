
app.runFile("./tests/fixtures/hello_world.js")
app.command("close_tab");

app.runFile("./tests/fixtures/multi_cursor.js")
app.command("close_tab");

app.command("quit");
