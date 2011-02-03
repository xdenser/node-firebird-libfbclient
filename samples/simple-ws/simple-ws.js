var fb=require('firebird');
var http=require('http');
var sys =require('sys');
var con = fb.createConnection();

con.connectSync('test.fdb','sysdba','masterkey','');

http.createServer(function(req,res){
  var rows = con.querySync("select * from rdb$relations")
                .fetchSync('all',true);
  res.writeHead(200,{'Content-Type':'text/plain'});
  res.end(sys.inspect(rows));  
}).listen(8080);

console.log('Server is running at http://localhost:8080');