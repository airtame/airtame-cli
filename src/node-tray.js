var readline = require('readline');

var net = require('net');

var airtame_server = net.connect({port: 8004},
    function() {
      console.log('connected to airtame-server!');
    });

airtame_server.on('data', function(data) {
      console.log(data.toString());
      airtame_server.end();
});

airtame_server.on('end', function() {
      console.log('disconnected from server');
});

var commandsMap  = {
    "connect" : connect_f,
    "disconnect" : disconnect_f,
    "exit" : exit
};

var commandsArray = [];

for (var key in commandsMap) {
    if (commandsMap.hasOwnProperty(key)) {
        commandsArray.push(key);
    }
}

var rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout,
    completer:completer
});

rl.setPrompt('airtame> ');
rl.prompt();
rl.on('line', function (cmd) {
        var command = cmd.split(" ")[0];
        var params = cmd.split(" ").slice(1, cmd.length);
        if (typeof commandsMap[command] != 'undefined') {
        commandsMap[command](params);
        }
        rl.prompt();
        });


function completer(line) {
    completions = commandsArray;
    var hits = completions.filter(function(c) { return c.indexOf(line) == 0 })
    return [hits.length ? hits : completions, line]
}

rl.on('SIGINT', function() {
        console.log("Ctrl+C");
        exit();
        });

function exit() {
    console.log("bye");
    process.exit();
}

function connect_f(params) {
    params.unshift('connect');
    msg = JSON.stringify(params);
    console.log("going to connect " + msg);
    airtame_server.write(JSON.stringify(msg));
}

function disconnect_f() {
    console.log("going to disconnect");
}

function settings(params) {
    console.log("setting ", params);
}

