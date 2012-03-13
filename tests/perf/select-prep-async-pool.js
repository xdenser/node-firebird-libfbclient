/*
Copyright by Denys Khanzhiyev
See license text in LICENSE file
*/

// Load configuration
var cfg = require("../config").cfg;
var fb = require('../../firebird');
var util = require('util');
var events = require('events');

var http = require('http');

function StatementPool()
{
    events.EventEmitter.call(this);
    this.conns = [];
    this.busy = [];
    this.MaxConns = 5;
    this.newConn = function(){
        var c ={
           conn: fb.createConnection()  
        };
        c.conn.connectSync(cfg.db, cfg.user, cfg.password, cfg.role);
        c.stmt = c.conn.prepareSync('select * from rdb$relations');
        this.conns.push(c); 
    };
    this.get = function(cb)
    {
        var self = this;
        var c = this.conns.pop();
        if(c) {
           this.busy.push(c); 
           cb(c);
        }
        else
        if((this.busy.length) >=this.MaxConns){
          this.once('release',function(){
           self.get(cb); 
          });
        }
        else {
            this.newConn();
            this.get(cb);
        }   
    };
    this.release = function(con){
        for(var i=0;i<this.busy.length;i++)
        {
            if(this.busy[i] == con){
                this.conns.push(this.busy[i]);
                this.busy.splice(i,1);
                var self = this;
                process.nextTick(function(){
                  self.emit('release');
                });  
                return;
            }
        }
    };
    
}
util.inherits(StatementPool, events.EventEmitter);

var pool = new StatementPool();
pool.setMaxListeners(2000);

    
http.createServer(function (req, res) {
    res.writeHead(200, {'Content-Type': 'text/plain'});
//    console.log(stmt);
    pool.get(function(con){
    var exec = function(){
      con.stmt.exec();
      con.stmt.once('result',function(err){
         //  var rows = [];
          res.write('[');
          con.stmt.fetch("all",true,function(r){
            //rows.push(r);
            res.write(JSON.stringify(r)+',');
          }, function(err){
            res.end(']');
            con.conn.commit(function(){
              pool.release(con);
            });
          });
      });
    };    
        
    var resp  = function(){
      if(!con.conn.inTransaction) con.conn.startTransaction(function(err){
          if(!err) exec();
      });
      else exec();     
    };
    
    
    resp();
    
  });  
    
}).listen(1337, "127.0.0.1");
console.log('Server running at http://127.0.0.1:1337/');

process.on('exit',function(){
     con.disconnect();   
});