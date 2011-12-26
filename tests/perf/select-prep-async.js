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
    
    var resp  = function(){
      console.log('resp');  
      if(stmt.lock){
        stmt.once('unlock',resp);
        return;  
      } 
      stmt.lock = true;
      console.log('resp start');
      
      stmt.once('result',function(err){
          var rows = [];
          if(err) console.log('exec error '+err.message);
          console.log('before fetch');
          stmt.fetch("all",true,function(r){
            rows.push(r);
          }, function(err){
            if(err) console.log('error '+err.message);
            console.log('after fetch ' +rows.length);
            res.end(util.inspect(rows));
            stmt.lock = false;
            process.nextTick(function(){
              stmt.emit('unlock');
            });
          });
      });
      stmt.exec();

    }
    
    
    resp();
    
}).listen(1337, "127.0.0.1");
console.log('Server running at http://127.0.0.1:1337/');

process.on('exit',function(){
     con.disconnect();   
});