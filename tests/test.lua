
run_file("./tests/fixtures/hello_world.lua")
command("close_tab");

run_file("./tests/fixtures/multi_cursor.lua")
command("close_tab");

command("quit");
