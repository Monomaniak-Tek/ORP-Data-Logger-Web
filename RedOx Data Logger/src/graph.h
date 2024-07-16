#ifndef GRAPH_H
#define GRAPH_H

#include <Arduino.h>

extern const char GRAPH_page[] PROGMEM;

#endif
const char GRAPH_page[] PROGMEM = R"====(
<!doctype html>
<html>
<head>
  <meta charset="UTF-8">
  <title>Graphique des Données</title>
  <script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/3.7.1/chart.min.js"></script>
</head>
<body>
  <div style="text-align:center;">
    <b>Graphique des Données</b><br>Veuillez sélectionner un fichier pour afficher les données sous forme de graphique.
  </div>
  <div>
    <form id="fileForm">
      <label for="fileSelect">Sélectionnez un fichier :</label>
      <select id="fileSelect" name="file">
        <!-- Les options de fichier seront ajoutées ici par JavaScript -->
      </select>
      <button type="button" onclick="loadFile()">Charger le fichier</button>
    </form>
  </div>
  <div class="chart-container" style="position: relative; height:500px; width:100%">
    <canvas id="myChart"></canvas>
  </div>
  <script>
    var ctx = document.getElementById('myChart').getContext('2d');
    var chart = new Chart(ctx, {
      type: 'line',
      data: {
        labels: [], // Placeholder for timestamps
        datasets: [{
          label: 'Valeur Red/Ox',
          backgroundColor: 'rgba(243, 156, 18, 0.5)',
          borderColor: 'rgba(243, 156, 18, 1)',
          data: [], // Placeholder for values
        }]
      },
      options: {
        scales: {
          xAxes: [{
            type: 'time',
            time: {
              parser: 'YYYY-MM-DD HH:mm:ss',
              tooltipFormat: 'll HH:mm',
              unit: 'minute',
              unitStepSize: 5,
              displayFormats: {
                'minute': 'YYYY-MM-DD HH:mm:ss'
              }
            },
            scaleLabel: {
              display: true,
              labelString: 'Time'
            }
          }],
          yAxes: [{
            scaleLabel: {
              display: true,
              labelString: 'Valeur'
            }
          }]
        }
      }
    });

    function updateChart(data) {
      var lines = data.trim().split('\n');
      var labels = [];
      var values = [];

      lines.forEach(line => {
        var parts = line.split(';');
        var timestamp = `${parts[0]}-${parts[1].padStart(2, '0')}-${parts[2].padStart(2, '0')} ${parts[3].padStart(2, '0')}:${parts[4].padStart(2, '0')}:${parts[5].padStart(2, '0')}`;
        labels.push(timestamp);
        values.push(parts[6]);
      });

      chart.data.labels = labels;
      chart.data.datasets[0].data = values;
      chart.update();
    }

    function loadFile() {
      var fileSelect = document.getElementById('fileSelect');
      var fileName = fileSelect.value;
      fetch('/readFile?file=' + fileName)
        .then(response => response.text())
        .then(data => updateChart(data))
        .catch(error => console.error('Error:', error));
    }

    window.onload = function() {
      fetch('/listFiles')
        .then(response => response.json())
        .then(files => {
          var fileSelect = document.getElementById('fileSelect');
          fileSelect.innerHTML = ""; // Clear existing options
          files.forEach(file => {
            var option = document.createElement('option');
            option.value = file;
            option.text = file;
            fileSelect.add(option);
          });
        })
        .catch(error => console.error('Error:', error));
    };
  </script>
</body>
<p><a href="/">Lecture en temps réel</a></p>
 <footer>
    <p style="text-align: right;">Benjamin B - Auteur, Date juillet 2024 - Copyleft</p>
  </footer>
</html>
)====";