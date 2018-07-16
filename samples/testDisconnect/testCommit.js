var cfg = require("./config").cfg;

// Require modules
var
  fb_binding = require("../../firebird.js");



  var conn = fb_binding.createConnection();
  conn.connectSync(cfg.db, cfg.user, cfg.password, cfg.role);
  if(conn.connected)  { 
    console.log("Connected to database");
  }
  var res = conn.querySync("select * from rdb$relations");
  console.log("Committin"); 
  conn.commit(function(err){
     console.log("Commit done ", err);
  });

setTimeout(function(){
  console.log("after timeout");
  
}, 5000);

