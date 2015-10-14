/*
  Requires the play-sound library:  https://github.com/shime/play-sound
*/

var express = require('express');
var player = require('play-sound')( opts = {} );

var router = express.Router();
var nodes = [];

/* GET home page. */
router.route('/')
    .get( function ( req, res ) {
          res.json( {} );
    });

router.route( '/event' )
    .post( function ( req, res ) {
        // Request body contains node ID and notification type
        var nodeId = req.body.nodeId;
        var notification = req.body.event;
        console.log( "Received event: " + nodeId );

        player.play('../../sounds/bell.wav', function (err) {} );

        res.json({ response: "ok" } );
    });

module.exports = router;
