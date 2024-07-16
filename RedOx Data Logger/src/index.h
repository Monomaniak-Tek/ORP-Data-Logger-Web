const char MAIN_page[] PROGMEM = R"=====(
<!doctype html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Beybleur | Red/Ox Visualiseur</title>
    <!--For offline ESP graphs see this tutorial https://circuits4you.com/2018/03/10/esp8266-jquery-and-ajax-web-server/ -->
    <script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.7.3/Chart.min.js"></script>    
    <style>
        canvas{
            -moz-user-select: none;
            -webkit-user-select: none;
            -ms-user-select: none;
        }
        /* Data Table Styling */
        #dataTable {
            font-family: "Trebuchet MS", Arial, Helvetica, sans-serif;
            border-collapse: collapse;
            width: 100%;
        }
        #dataTable td, #dataTable th {
            border: 1px solid #ddd;
            padding: 8px;
        }
        #dataTable tr:nth-child(even){background-color: #f2f2f2;}
        #dataTable tr:hover {background-color: #ddd;}
        #dataTable th {
            padding-top: 12px;
            padding-bottom: 12px;
            text-align: left;
            background-color: #4CAF50;
            color: white;
        }
    </style>
</head>
<body>
    <div style="text-align:center;">
        <b>Piscine de Beybleu</b><br>Données en temps reel de la sonde Red/Ox
    </div>
    <div class="chart-container" style="position: relative; height:700px; width:100%">
        <canvas id="Chart" width="400" height="400"></canvas>
    </div>
    <div>
        <table id="dataTable">
            <tr><th>Heure</th><th>Valeur Red-Ox</th></tr>
        </table>
    </div>
    <br>
    <br>
    <h3>Fichiers sur la carte SD</h3>
    <ul id="fileList"></ul>
    <script>
        //Graphes
        var values = [];
        var timeStamp = [];

        function showGraph() {
            for (i = 0; i < arguments.length; i++) {
                values.push(arguments[i]);
            }
            var ctx = document.getElementById("Chart").getContext('2d');
            var Chart2 = new Chart(ctx, {
                type: 'line',
                data: {
                    labels: timeStamp,
                    datasets: [{
                        label: "Voltage (mV)",
                        fill: false,
                        backgroundColor: 'rgba( 243, 156, 18 , 1)',
                        borderColor: 'rgba( 243, 156, 18 , 1)',
                        data: values,
                    }],
                },
                options: {
                    title: {
                        display: true,
                        text: "Valeur Red/Ox (550 mV < eau désinfectante < 750 mV)"
                    },
                    maintainAspectRatio: false,
                    elements: {
                        line: {
                            tension: 0.5
                        }
                    },
                    scales: {
                        yAxes: [{
                            ticks: {
                                beginAtZero:true
                            }
                        }]
                    }
                }
            });
        }

        window.onload = function() {
            console.log(new Date().toLocaleTimeString());
            showGraph(5, 10, 4, 58);
            listFiles();
        };

        setInterval(function() {
            getData();
        }, 5000);

        function getData() {
            var xhttp = new XMLHttpRequest();
            xhttp.onreadystatechange = function() {
                if (this.readyState == 4 && this.status == 200) {
                    var time = new Date().toLocaleTimeString();
                    var ADCValue = this.responseText;
                    values.push(ADCValue);
                    timeStamp.push(time);
                    showGraph();
                    var table = document.getElementById("dataTable");
                    var row = table.insertRow(1);
                    var cell1 = row.insertCell(0);
                    var cell2 = row.insertCell(1);
                    cell1.innerHTML = time;
                    cell2.innerHTML = ADCValue;
                }
            };
            xhttp.open("GET", "/readADC", true);
            xhttp.send();
        }

        function listFiles() {
            var xhttp = new XMLHttpRequest();
            xhttp.onreadystatechange = function() {
                if (this.readyState == 4 && this.status == 200) {
                    var files = JSON.parse(this.responseText);
                    var fileList = document.getElementById("fileList");
                    fileList.innerHTML = "";
                    files.forEach(function(file) {
                        var li = document.createElement("li");
                        var link = document.createElement("a");
                        link.href = "/download?file=" + file;
                        link.innerText = file;
                        li.appendChild(link);
                        fileList.appendChild(li);
                    });
                }
            };
            xhttp.open("GET", "/listFiles", true);
            xhttp.send();
        }
    </script>
</body>
 <p><a href="/graph">Visualiser l'historique des graphiques</a></p>
 <footer>
    <p style="text-align: right;">Benjamin B - Auteur, Date juillet 2024 - Copyleft</p>
  </footer>
</html>


)=====";