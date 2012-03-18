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
    this.newConn = function(cb){
        var c ={
           conn: fb.createConnection()  
        }, self = this;
        
        c.conn.connect(cfg.db, cfg.user, cfg.password, cfg.role, function(){
          c.stmt = c.conn.prepareSync('select * from rdb$relations');
       // c.stmt = c.conn.prepareSync('select first 5 * from test_t where pid = ?');
          self.conns.push(c);
          cb(); 
        });
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
            this.newConn(function(){
              this.get(cb);
            });
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
    pool.get(function(con){
    var exec = function(){
      con.stmt.exec(10);
      con.stmt.once('result',function(err){
          res.write('[');
          con.stmt.fetch("all",true,function(r){
            res.write(JSON.stringify(r)+',');
          }, function(err){
            res.end(']');
            con.conn.commit(function(){
              pool.release(con);
            });
          });
      });
    };    
        
   if(!con.conn.inTransaction) con.conn.startTransaction(function(err){
          if(!err) exec();
   });
   else exec();     
  });  
    
}).listen(1337, "127.0.0.1");
console.log('Server running at http://127.0.0.1:1337/');

process.on('exit',function(){
     con.disconnect();   
});