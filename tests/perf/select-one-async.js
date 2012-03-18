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
    con.connectSync(cfg.db, cfg.user, cfg.password, cfg.role),
    busy = false,
    next = [];

http.createServer(function (req, res) {
    res.writeHead(200, {'Content-Type': 'text/plain'});
                
    function doReq(){
       //con.query('select * from rdb$relations',function(err,rs){
       con.query('select first 20 * from test_t where pid = 10',function(err,rs){
          res.write('[');
          rs.fetch("all",true,function(r){
           res.write(JSON.stringify(r)+',');
          }, function(err){
            res.end(']');
            con.commit(function(){
              busy = false;
              var n = next.pop();
              if(n) n();
            });
          });
       });
    }
    
    function process(){
       busy = true;     
       if(!con.inTransaction) con.startTransaction(doReq);
       else doReq(); 
    }   
     
   if(busy) next.push(function(){
      process();  
   }); 
   else process();
    
}).listen(1337, "127.0.0.1");
console.log('Server running at http://127.0.0.1:1337/');

process.on('exit',function(){
     con.disconnect();   
});