import { createSignal, onMount } from 'solid-js';
import { SolidApexCharts } from 'solid-apexcharts';

function App() {
  return (
    <div class="w-screen">
      <Header />
      <Container />
    </div>
  );
}

function Header() {
  return (
    <div class="w-screen bg-green-900 p-4 shadow-lg shadow-dark-900">
      <h1 class="text-4xl text-white">Ausyx</h1>
    </div>
  );
}

function Container() {
  const [loteID, setLoteID] = createSignal(1);

  return (
    <div class="w-screen p-10 bg-cool-gray-600">
      <ContainerHeader loteID={loteID} setLoteID={setLoteID} />
      <ContainerContent loteID={loteID} setLoteID={setLoteID} />
    </div>
  );
}

function ContainerHeader(props) {
  return (
    <Show
      when={props.loteID() == -1}
      fallback={
        <div class="w-full p-4 bg-slate-400 text-center">
          <button onClick={() => props.setLoteID(-1)}>Voltar</button>
          <h1 class="text-3xl">Lote {props.loteID()}</h1>
        </div>
      }>
      <div class="w-full p-4 bg-slate-400 text-center">
        <h1 class="text-3xl">Histórico de Lotes</h1>
      </div>
    </Show>
  );
}

function ContainerContent(props) {
  return (
    <div class="w-full p-4 bg-slate-200">
      <Show
        when={props.loteID() == -1}
        fallback={<Lote loteID={props.loteID} />}>
        <LotesList setLoteID={props.setLoteID} />
      </Show>
    </div>
  );
}

function Lote(props) {
  const [loteData, setLoteData] = createSignal([]);
  const [series, setSeries] = createSignal({
    list: [
      {
        name: 'Entrada',
        data: [24, 27, 28, 27, 26, 25, 24, 23, 22]
      },
      {
        name: 'Massa 1',
        data: [1, 2, 3]
      },
      {
        name: 'Massa 2',
        data: []
      }
    ]
  });
  const [options, setOptions] = createSignal({
    chart: {
      height: 350,
      type: 'area'
    },
    dataLabels: {
      enabled: false
    },
    stroke: {
      curve: 'smooth'
    },
    xaxis: {
      type: 'datetime',
      categories: [
        '2022-06-23 00:49:18',
        '2022-06-23 00:50:00',
        '2022-06-23 00:50:11'
      ]
    },
    tooltip: {
      x: {
        format: 'dd/MM/yyyy HH:mm'
      }
    }
  });

  onMount(async () => {
    let headers = new Headers();

    headers.append('Content-Type', 'application/json');
    headers.append('Accept', 'application/json');

    // headers.append('GET');

    fetch(`http://192.168.4.1/lote/${props.loteID()}`, {
      mode: 'no-cors',
      method: 'GET',
      headers: headers
    })
      .then(response => response.text())
      .then(data => {
        console.log(data);
        data = data.replace(/ /g, '');
        const rows = data.split(';');

        let newSeries = {
          list: [
            {
              name: 'Entrada',
              data: []
            },
            {
              name: 'Massa 1',
              data: []
            },
            {
              name: 'Massa 2',
              data: []
            }
          ]
        };

        let newOptions = {
          chart: {
            height: 350,
            type: 'area'
          },
          dataLabels: {
            enabled: false
          },
          stroke: {
            curve: 'smooth'
          },
          xaxis: {
            type: 'datetime',
            categories: []
          },
          tooltip: {
            x: {
              format: 'dd/MM/yyyy HH:mm'
            }
          }
        };

        let oldEntrada = 0;
        let oldMassa1 = 0;
        let oldMassa2 = 0;
        for (let i = 0; i < rows.length; i++) {
          const row = rows[i];
          let data = row.split(',');
          let date = data[0];
          let target = data[1];
          let value = data[2];

          if (target == 'SENSOR_ENTRADA') {
            oldEntrada = value;
          } else if (target == 'SENSOR_MASSA_1') {
            oldMassa1 = value;
          } else if (target == 'SENSOR_MASSA_2') {
            oldMassa2 = value;
          }

          newOptions.xaxis.categories.push(date);
          newSeries.list[0].data.push(oldEntrada);
          newSeries.list[1].data.push(oldEntrada);
          newSeries.list[2].data.push(oldEntrada);
        }
      })
      .catch(error => console.log('Authorization failed : ' + error.message));
  });

  return (
    <SolidApexCharts
      width="500"
      type="area"
      options={options()}
      series={series().list}
    />
  );
}

function LotesList(props) {
  const [lotes, setLotes] = createSignal([]);

  onMount(async () => {
    let headers = new Headers();

    headers.append('Content-Type', 'application/json');
    headers.append('Accept', 'application/json');

    // headers.append('GET');

    fetch('http://192.168.4.1/lotes', {
      mode: 'no-cors',
      method: 'GET',
      headers: headers
    })
      .then(response => response.text())
      .then(data => {
        console.log(data);
        let loteArray = data.split(',');
        console.log(loteArray);
        setLotes(loteArray);
      })
      .catch(error => console.log('Authorization failed : ' + error.message));
  });

  return (
    <ul>
      <For each={lotes()}>
        {(lote, i) => (
          <li>
            <button onClick={() => props.setLoteID(Number(lote))}>
              Lote {lote}
            </button>
          </li>
        )}
      </For>
    </ul>
  );
}
export default App;
