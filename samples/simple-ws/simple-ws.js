var fb=require('../../firebird');
var http=require('http');
var sys =require('sys');
// Create Firebird Connection Object
var con = fb.createConnection();

// Connect to Database
con.connectSync('test.fdb','sysdba','masterkey','');

// Create HTTP server
http.createServer(function(req,res){
  // Query and fecth all rows
  var rows = con.querySync("select * from rdb$relations")
                .fetchSync('all',true);
  res.writeHead(200,{'Content-Type':'text/plain'});
  // Return rows object (it is array) to browser
  res.end(sys.inspect(rows));  
}).listen(8080);

console.log('Server is running at http://localhost:8080');