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
var stmt = con.prepareSync('select * from rdb$relations');
    stmt.setMaxListeners(2000);
    stmt.lock = false;
    
http.createServer(function (req, res) {
    res.writeHead(200, {'Content-Type': 'text/plain'});
//    console.log(stmt);
    var exec = function(){
      stmt.exec();
      stmt.once('result',function(err){
          var rows = [];
          stmt.fetch("all",true,function(r){
            rows.push(r);
          }, function(err){
            res.end(util.inspect(rows));
            con.commit(function(err){
             //if(err) return;   
             stmt.lock = false;
             process.nextTick(function(){
               stmt.emit('unlock');
             });
            });
          });
      });
    }; 
    
    var resp  = function(){
      if(stmt.lock){
        stmt.once('unlock',resp);
        return;  
      } 
      stmt.lock = true;
      if(!con.inTransaction) con.startTransaction(function(err){
          if(!err) exec();
      });
      else exec();
    }
    
    
    resp();
    
}).listen(1337, "127.0.0.1");
console.log('Server running at http://127.0.0.1:1337/');

process.on('exit',function(){
     con.disconnect();   
});