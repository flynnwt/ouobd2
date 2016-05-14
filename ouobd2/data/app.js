//(function () { // not for testing
'use strict';

// these arent working.  try clipboard.js - works
new Clipboard('#rawLogClipboard'); 

function copyToClipboard(el) {
  var $temp;
  $temp = $('<input>');
  $('#temp').append($temp);
  $temp.val(el.text()).select();
  console.log($temp);
  console.log($temp.val());
  console.log($temp.val().length);
  console.log($temp[0].selectionStart);
  console.log($temp[0].selectionEnd);
  console.log(window.getSelection());
  console.log(window.getSelection().toString());
  console.log(document.execCommand('copy'));
  //$temp.remove();
}


function copyToClipboard2(text) {
  var input = document.createElement('input');
  //input.style.position = 'fixed';
  //input.style.opacity = 0;
  input.value = text;
  document.body.appendChild(input);
  input.select();
  document.execCommand('Copy');
  //document.body.removeChild(input);
};


function uploadFileText(file) {
  var reader = new FileReader();
  var xhr = new XMLHttpRequest();
  var boundary = 'wtfwtfwtf';

  xhr.open('POST', 'http://' + location.host + '/upload');
  xhr.setRequestHeader('Content-Type', 'multipart/form-data; boundary=' + boundary);
  xhr.onreadystatechange = function() {
    if (xhr.readyState === 4) {
      console.log('upload done: ' + xhr.status);
    }
    if (xhr.readyState == 4 && xhr.status == 200) {
      $('.uploadStatus').html('');
    }
  };
  reader.onload = function(evt) {
    var filename, content, data;

    //filename = file.name;
    filename = $('#fileInputTextName').val();
    console.log(file, filename);
    content = evt.target.result;
    data = '--' + boundary + '\r\nContent-Disposition: form-data; name="files"; filename="' + filename + '"\r\nContent-Type: text/plain\r\n\r\n' + content + '\r\n--' + boundary + '--\r\n';
    //console.log(data);
    $('.uploadStatus').html('Uploading...');
    xhr.send(data);
  };
  reader.readAsText(file);
}

var chartRPM = c3.generate({
  bindto: '#chartRPM',
  axis: {
    x: {
      show: false,
    },
    y: {
      min: 0,
      padding: {
        bottom: 0
      },
      label: {
        text: 'RPM',
        position: 'outer-middle'
      },
      format: function(e, d) {
        if (e < 0) { // when faking chart
          return 0;
        } else {
          return e;
        }
      },
      tick: {
        count: 5,
        values: [1000, 3000, 5000, 8000, 15000]
      }
    }
  },
  data: { x: 'x', columns: [], type: 'spline' },
  point: {
    show: false
  },
  grid: {
    x: {
      show: true
    },
    y: {
      show: true
    }
  },
  legend: {
    show: false
  }
});

function updateChart(times, values) {
  chartRPM.load({ columns: [['x'].concat(times), ['y'].concat(values)] })
}

var i, x, y, now = new Date().getTime(), last = 3200;
var times = [], values = [];
for (i = 0; i < 50; i++) {
  times.push(i);
  last = last + (i * 4) + (Math.random() * 500 * (Math.random() < .5 ? -1 : 1));
  values.push(last);
}
updateChart(times, values);

function updateReadyState(state) {
  if (state) {
    $('.status .state').removeClass('pending').removeClass('notready').addClass('ready').text('Active');
    $('#inputCmd').prop('disabled', false);
    $('#logActionsBtn').prop('disabled', false);
    $('#vehicle').prop('disabled', false);
    $('#vehicleActionsBtn').prop('disabled', false);
  } else {
    $('.status .state').removeClass('pending').removeClass('ready').addClass('notready').text('Not Ready');
    $('#inputCmd').prop('disabled', true);
    $('#logActionsBtn').prop('disabled', true);
    $('#vehicle').prop('disabled', true);
    $('#vehicleActionsBtn').prop('disabled', true);
  }
}

var ws, log, wsPort = 81;

function sendCmd(cmd) {
  var msg = {
    type: 'cmd',
    value: cmd
  };
  ws.send(JSON.stringify(msg));
}

$(document).ready(function() {
  var i;

  $('#info').hide(); $('#chartWatts').hide();
  $('#status')
    .mouseover(function(e) {
      $('#info').show();
    })
    .mouseout(function(e) {
      $('#info').hide()
    });
  //latest(); setInterval(latest, 6000);
  //systemInfo(); setInterval(systemInfo, 10000);
  log = new Log();
  log.el($('.log-data'));
  log.add('log started...');

  updateReadyState(false);

  ws = new Socket(location.hostname, wsPort);
  ws.ready(updateReadyState);
  // could also implement these as a log property that is true or false, and just use the recvFcn to
  //  decide what to do based on the other properties
  ws.on('log', function(msg) {
    log.add(msg);
  });
  ws.on('status', function(msg) {
    log.add('status', msg);
    $('#deviceName').text(msg.device);
  });
  ws.on('init', function(msg) {
    log.add('init: ' + msg);
  });
  ws.on('reset', function(msg) {
    log.add('reset: ' + msg);
  });
  ws.on('rawlog', function(o) {
    // logging, data
    $('#rawLogEnabled').text(o.logging? 'Yes' : 'No');
    $('#rawLogData').html(o.data);
    $('#rawLogData').stop().animate({
      scrollTop: $('#rawLogData').prop('scrollHeight')
    }, 2000);
  });

  $('.status .state').removeClass('notready').removeClass('ready').addClass('pending');
  ws.start();

  // if i want to start/stop ws - prob only for testing;
  $('#wsConnect').click = ws.start;
  $('#wsClose').click = ws.stop;

    // Set up Main controls
    $('.status').click(function() {
    if (ws.statusReady) {
      ws.stop();
    } else {
      $('.status .state').removeClass('notready').removeClass('ready').addClass('pending');
      ws.start();
    }
  });

  // would load this dynamically with data from server (previously filled form data, history, etc.)
  var vehicleActions = ['Blue Sky', 'Krazy Kab', 'Thunderkiss59', 'Sambuca'];
  for (i = 0; i < vehicleActions.length; i++) {
    $('#vehicleActions').append('<li id="vehicleAction_' + i + '"><a href="#">' + vehicleActions[i] + '</a></li>');
    (function(i) {
      $('#vehicleAction_' + i).click(function() {
        $('#vehicle').val(vehicleActions[i]);
        sendCmd('vehicle ' + vehicleActions[i]);
      });
    })(i);
  }


  // Set up Log controls
  $('#logPause').click(function() {
    if ($(this).hasClass('paused')) {
      log.autoscroll = 5000;
      $(this).removeClass('paused');
      $(this).removeClass('btn-success');
      $(this).addClass('btn-warning');
      $(this).html('<span class="glyphicon glyphicon-pause"></span>');
    } else {
      log.autoscroll = 0;
      $(this).addClass('paused');
      $(this).removeClass('btn-warning');
      $(this).addClass('btn-success');
      $(this).html('<span class="glyphicon glyphicon-play"></span>');
    }
  });
  $('#logClear').click(function() {
    log.clear();
  });

  // could load this dynamically with data from server
  var logActions = ['status', 'init', 'reset'];
  for (i = 0; i < logActions.length; i++) {
    $('#logActions').append('<li id="logAction_' + i + '"><a href="#">' + logActions[i] + '</a></li>');
    (function(i) {
      $('#logAction_' + i).click(function() {
        sendCmd(logActions[i]);
      })
    })(i);
  }

  $('#sendCmd').click(function() {
    sendCmd($('#inputCmd').val());
    $('#inputCmd').val('');
  });
  $('#inputCmd').keypress(function(event) {
    if (event.keyCode == 13) {
      sendCmd($('#inputCmd').val());
      $('#inputCmd').val('');
    }
  });

  $('#clearCmd').click(function() {
    $('#inputCmd').val('');
  });


  // upload testing...
  $('#fileInputText').change(function() {
    var file = $(this)[0].files[0];
    console.log('change');
    console.log(file.name, file.size, file.type);
    $('#fileInputTextName').val(file.name);
  });

  $('#fileUploadText').click(function() {
    var file = $('#fileInputText')[0].files[0];
    console.log('click');
    uploadFileText(file);
  });

  // Set up Raw Log controls
  $('#rawLogRefresh').click(function() {
    sendCmd('getlog');
  });
  $('#rawLogClear').click(function() {
    sendCmd('clearlog');
    sendCmd('getlog');
  });
  $('#rawLogCopy').click(function() {
    //console.log(document.queryCommandSupported('copy'));
    copyToClipboard($('#rawLogData'));
  });

});

/*
   * WebSocket
   * receive, parse, and distribute incoming (json) data
   * send outgoing data
   */
var Socket = function(u, p) {
  this.url = 'ws://' + u + ':' + p;
  this.ws = null;
  this.received = 0;
  this.sent = 0;
  this.on = [];
  this.statusReady = false;
  this.openFcn = null;
  this.readyFcn = null;
  this.recvFcn = null;

  var me = this;

  this.start = function() {

    console.log('Opening ' + me.url + '...');
    me.ws = new WebSocket(me.url);
    me.ws.binaryType = 'arraybuffer';

    me.ws.onopen = function(evt) {
      console.log('Opened', evt);
      me.statusReady = true;
      if (me.openFcn) {
        (me.openFcn)(true);
      }
      if (me.readyFcn) {
        (me.readyFcn)(me.statusReady);
      }
    };

    me.ws.onerror = function(evt) {
      console.log('Error', evt);
    };

    me.ws.onmessage = function(evt) {
      var o, json;

      me.received++;
      console.log('recv (' + me.received + ')', evt);
      // check for handlers for properties; any property with an 'on' handler will be called with its data
      //for (var i = 0; i < evt.data.length; i++) {
      //  console.log(evt.data.substring(i, 1), evt.data.substring(i, 1));
      //}
      try {
        json = JSON.parse(evt.data);
        if (me.recvFcn) {
          (me.recvFcn(json));
        }
        for (o in json) {
          if (me.on[o]) {
            (me.on[o])(json[o]);
          }
        }
      } catch (e) {
        console.log(e);
      }
    };

    me.ws.onclose = function(evt) {
      me.ws = null;
      me.statusReady = false;
      // do ready() too?  onclose can be called for reasons besides stop() - or only if ready was true?
      if (me.readyFcn) {
        (me.readyFcn)(me.statusReady);
      }
      if (me.openFcn) {
        (me.openFcn)(false);
      }
      console.log('Closed', evt);
    };
  };

  this.stop = function() {
    console.log('Closing');
    me.statusReady = false;
    if (me.readyFcn) {
      (me.readyFcn)(me.statusReady);
    }
    me.ws.close();
  };

  this.send = function(data) {
    if (me.statusReady) {
      me.sent++;
      console.log('send (' + me.sent + ')', data);
      try {
        me.ws.send(data);
      } catch (e) {
        console.log(e);
      }
    }
  };

  this.ready = function(f) {
    this.readyFcn = f;
  }

  this.open = function(f) {
    this.openFcn = f;
  }

  this.receive = function(f) {
    this.recvFcn = f;
  }

  this.on = function(type, handler) {
    this.on[type] = handler;
  };

  this.off = function(type) {
    this.on[type] = undefined;
  }

};

var Log = function() {
  this.items = [];
  this.el = null;
  this.autoscroll = 5000;  // ms to animate; 0 = no autoscroll

  this.add = function(msg, obj) {
    var o;

    if (obj) {
      msg += ': {';
      for (o in obj) {
        msg += '<div class="logprop">' + o + ': "' + obj[o] + '"</div>';
      }
      msg += '}';
    }
    this.items.push(msg);
    if (this.el) {
      //this.el.prepend('<p>' + msg + '<p>');
      this.el.append('<p>' + msg + '<p>');
      if (this.autoscroll) {
        this.el.stop().animate({
          scrollTop: this.el.prop('scrollHeight')
        }, this.autoscroll);
      }
    }
  }

  this.clear = function() {
    this.items = [];
    if (this.el) {
      this.el.html('');
    }
  }

  this.el = function(e) {
    this.el = e;
  }

}

//})();