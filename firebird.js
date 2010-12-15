var binding = require("./build/default/binding");

var Connection = binding.Connection;


exports.createConnection = function () {
  var c = new Connection;
  return c;
};
