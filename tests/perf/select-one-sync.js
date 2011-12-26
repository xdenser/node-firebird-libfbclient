/*
Copyright by Denys Khanzhiyev
See license text in LICENSE file
*/

// Load configuration
var cfg = require("../config").cfg;
var fb = require('../../firebird');
var util = require('util');

var http = require('http');

var con = fb.createConnection();
    con.connectSync(cfg.db, cfg.user, cfg.password, cfg.role);

http.createServer(function (req, res) {
    res.writeHead(200, {'Content-Type': 'text/plain'});
    if(!con.inTransaction) con.startTransactionSync();
    var rs = con.querySync('select * from rdb$relations');
    var rows = rs.fetchSync("all",true);
    res.end(util.inspect(rows));
    con.commitSync();
}).listen(1337, "127.0.0.1");
console.log('Server running at http://127.0.0.1:1337/');

process.on('exit',function(){
   con.disconnect();
});