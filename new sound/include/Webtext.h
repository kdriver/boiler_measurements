
const char graph[] PROGMEM = R"rawliteral(
 <!DOCTYPE HTML><html><head>
    <title>Boiler readings</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <meta http-equiv="refresh" content="20">
    </head>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.7.2/Chart.js"></script>
    <body>
    <canvas id="myChart" width="400" height="400"></canvas>
<script>
    var ctx = document.getElementById('myChart').getContext('2d');
    ctx.canvas.parentNode.style.height = "500px";
    var myChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: %s,
            datasets: [{
                label: 'sound level',
                borderColor: 'green',
                data: %s
            },
            {
              label: 'moving average',
              borderColor: 'blue',
               data: %s
            },
            {
              label: 'kalman ',
              borderColor: 'red',
               data: %s
            }
            ]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
                yAxes: [{
                    ticks: {
                        beginAtZero: true
                    }
                }]
            }
        }
    });
    </script>
    </body>
    </html>)rawliteral";

/*----------------------------------------------------------------
 <form action="/get">
    On Threshold(unused)  : <input type="text" name="boiler_on_threshold">
    <input type="submit" value="Submit"> current = %d
  </form><br>
  */
 
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head>
  <body>
  current moving average is %f , boiler_status is %s
  </p>
 
  <form action="/get">
    How often sample is taken (mS)   : <input type="text" name="loop_delay">
    <input type="submit" value="Submit"> current = %d
  </form><br>
  <form action="/get">
    How long a sample is taken for (mS) : <input type="text" name="sample_period">
    <input type="submit" value="Submit"> current = %d
  </form><br>
  <form action="/get">
    number of samples for moving average : <input type="text" name="sample_average">
    <input type="submit" value="Submit"> current = %d
  </form><br>
  <form action="/get">
    Peak to Peak threshold for boiler on : <input type="text" name="boiler_on_threshold_1">
    <input type="submit" value="Submit"> current = %d
  </form><br>

   <form action="/get">
    Kalman Measurement Uncertainty       : <input type="text" name="measurement_uncertainty">
    <input type="submit" value="Submit"> current = %f
  </form><br>
     <form action="/get">
    Kalman Estimation Uncertainty        : <input type="text" name="estimation_uncertainty">
    <input type="submit" value="Submit"> current = %f
  </form><br>
   <form action="/get">
    Kalman noise : <input type="text" name="noise">
    <input type="submit" value="Submit"> current = %f
  </form><br>
  <p> Moving average calculated over %f seconds </p>
</body></html>)rawliteral";

const char* PARAM_INPUT_1 = "boiler_on_threshold";
const char* PARAM_INPUT_2 = "loop_delay";
const char* PARAM_INPUT_3 = "sample_period";
const char* PARAM_INPUT_4 = "sample_average";
const char* PARAM_INPUT_5 = "boiler_on_threshold_1";
const char* PARAM_INPUT_6 = "measurement_uncertainty";
const char* PARAM_INPUT_7 =  "estimation_uncertainty";
const char* PARAM_INPUT_8 = "noise";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}