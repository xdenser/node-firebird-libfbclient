/*
Copyright by Denys Khanzhiyev
See license text in LICENSE file
*/

// Load configuration
var cfg = require("../config").cfg;
var sys = require("sys");

var zexports = {};
// Require modules
var
  fb_binding = require("../../firebird");


exports.FetchFromSameConnection = function(test){
  var num_of_fetches = 100
  test.expect(1 + num_of_fetches );
  var conn = fb_binding.createConnection();
  conn.connectSync(cfg.db, cfg.user, cfg.password, cfg.role);
  test.ok(conn.connected,"Connected to database");
  var res = [];
  for(var i = 0; i<num_of_fetches; i++)
    res.push(conn.querySync("select * from rdb$relations"));
  
  var fetchCount= 0;
  
  function CheckDone(){
   if( fetchCount == num_of_fetches ) {
     test.done();
     conn.disconnect(); 
   }      
  }
  
  for(i in res){
    res[i].fetch("all",true,function(row){
    //+sys.inspect(row)
    // console.log('res1: ');
    }, function(err,eof){ 
    fetchCount++;
    if(err) console.log('Error ' + err);
    test.ok(eof,"res1 fetched");
    CheckDone(); 
    });
  }
          
}