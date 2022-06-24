import { createSignal, onMount, Show } from 'solid-js';
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
  const [loteID, setLoteID] = createSignal(-1);

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
      categories: []
    },
    tooltip: {
      x: {
        format: 'dd/MM/yyyy HH:mm:ss'
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
        for (let i = 0; i < rows.length - 1; i++) {
          const row = rows[i];
          let data = row.trim();
          data = data.split(',');
          let date = data[0].trim();
          let target = data[1].trim();
          let value = Number(data[2].trim());

          if (target == 'SENSOR_ENTRADA') {
            oldEntrada = value;
          } else if (target == 'SENSOR_MASSA_1') {
            oldMassa1 = value;
          } else if (target == 'SENSOR_MASSA_2') {
            oldMassa2 = value;
          } else if (target == 'CONEXAO_1') {
            newOptions.xaxis.categories.push(date);
            newSeries.list[0].data.push(oldEntrada);
            newSeries.list[1].data.push(oldMassa1);
            newSeries.list[2].data.push(oldMassa2);

            if (value == 0) {
              oldMassa1 = 0;
            }
          } else if (target == 'CONEXAO_2') {
            newOptions.xaxis.categories.push(date);
            newSeries.list[0].data.push(oldEntrada);
            newSeries.list[1].data.push(oldMassa1);
            newSeries.list[2].data.push(oldMassa2);

            if (value == 0) {
              oldMassa2 = 1;
            }
          }

          console.log(row);
          console.log(date);
          console.log(target);
          console.log(value);

          newOptions.xaxis.categories.push(date);
          newSeries.list[0].data.push(oldEntrada);
          newSeries.list[1].data.push(oldMassa1);
          newSeries.list[2].data.push(oldMassa2);
        }

        setOptions(newOptions);
        setSeries(newSeries);
      })
      .catch(error => console.log('Authorization failed : ' + error.message));
  });

  return (
    <SolidApexCharts
      width="100%"
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

        console.log(loteArray);
        if (loteArray[0] === '') {
          setLotes([]);
        } else {
          setLotes(loteArray);
        }
      })
      .catch(error => console.log('Authorization failed : ' + error.message));
  });

  return (
    <Show
      when={lotes() !== []}
      fallback={<h1 class="text-3xl">Histórico vazio! Inicie um Lote!</h1>}>
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
    </Show>
  );
}
export default App;
