// Load configuration
var cfg = require("./config").cfg;

// Require modules
var
  fb_binding = require("../../firebird.js");



  var conn = fb_binding.createConnection();
  conn.connectSync(cfg.db, cfg.user, cfg.password, cfg.role);
  if(conn.connected)  { 
    console.log("Connected to database");
  }
//  var trans =  conn.startNewTransactionSync();
   var res = conn.querySync("select * from rdb$relations");
//  trans.rollbackSync();
  console.log("Query result 1", res)

  
  setTimeout(function() {
         console.log("Try again")
	 var blob = conn.newBlobSync();
         console.log("Blob", blob);
          

  }, 20000);

