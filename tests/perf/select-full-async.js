/*
Copyright by Denys Khanzhiyev
See license text in LICENSE file
*/

// Load configuration
var cfg = require("../config").cfg;
var fb = require('../../firebird');
var util = require('util');

var http = require('http');

http.createServer(function (req, res) {
    res.writeHead(200, {'Content-Type': 'text/plain'});
    var con = fb.createConnection();
    con.connect(cfg.db, cfg.user, cfg.password, cfg.role,function(){
       con.query('select * from rdb$relations',function(err,rs){
          //var rows = [];
          res.write('[');
          rs.fetch("all",true,function(r){
            //rows.push(r);
             res.write(JSON.stringify(r)+',');
          }, function(err){
            con.disconnect();
            //res.end(util.inspect(rows));
             res.end(']');
          });
       
       });
    });
    
}).listen(1337, "127.0.0.1");
console.log('Server running at http://127.0.0.1:1337/');