var express = require('express');
var router = express.Router();
var nodes = [];

/* GET home page. */
router.route('/')
    .get( function (req, res) {
          res.json( { "count": nodes.length } );
    });

/* POST node registering */
router.route('/:nodeId')
  .post( function (req, res) {
      var nodeId = req.params.nodeId;
      nodes[nodeId] = "alive";
      res.json( { "status": "registered " + nodeId } )
  })
  .get( function (req, res) {
      var nodeId = req.params.nodeId;
      res.json( { "node": nodes[nodeId] } )
  });
  
module.exports = router;
