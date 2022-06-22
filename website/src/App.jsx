import { HopeProvider } from '@hope-ui/solid';
import { Container, Heading, Flex, Anchor } from '@hope-ui/solid';
import { createSignal, For } from 'solid-js';

import {
  Accordion,
  AccordionItem,
  AccordionButton,
  AccordionIcon,
  AccordionPanel,
  Text
} from '@hope-ui/solid';

function App() {
  return (
    <HopeProvider>
      <div>
        <Header />
        <Container backgroundColor="lightgrey" centerContent>
          <Heading size="3xl" align="content-center">
            Histórico de Lotes
          </Heading>

          <Lotes />
        </Container>
      </div>
    </HopeProvider>
  );
}

function Header() {
  return (
    <Flex backgroundColor="green" height="5em" alignItems="center">
      <Heading level="1" size="3xl" margin="20px" color="white">
        Ausyx
      </Heading>
    </Flex>
  );
}

function Lotes() {
  const [lotes, setLotes] = createSignal([{ number: 1 }]);
  return (
    <Accordion width="$full">
      <For each={lotes()}>
        {(lote, i) => (
          <AccordionItem>
            <h2>
              <AccordionButton>
                <Text flex={1} fontWeight="$medium" textAlign="start">
                  Lote {lote.number}
                </Text>
                <AccordionIcon />
              </AccordionButton>
            </h2>

            <AccordionPanel>
              <Anchor href={`localhost:3308/download/${lote.number}`}>Baixar</Anchor>
            </AccordionPanel>
          </AccordionItem>
        )}
      </For>
    </Accordion>
  );
}

export default App;
