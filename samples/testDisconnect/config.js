/*
Copyright by Denys Khanzhiyev 

See license text in LICENSE file
*/
const path = require('path');

exports.cfg = {
  // Database connection settings
  db: "127.0.0.1:" + path.join(__dirname,"..","..","build","test_db","TEST.FDB"),
//  db: "192.168.111.133:test.fdb",
  user: "sysdba",
  password: "masterkey",
  role: ""
};

console.log(exports.cfg);

