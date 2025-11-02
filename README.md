# Transmissão de Dados via LoRa

Este projeto implementa um sistema de comunicação sem fio entre uma *FPGA ColorLight i9* (atuando como *nó transmissor*) e uma *BitDogLab* (atuando como *nó receptor*), utilizando módulos LoRa RFM96 para transmissão de dados ambientais (temperatura e umidade).

O sistema é composto por:
- Um SoC customizado desenvolvido com LiteX, executando um firmware bare-metal em C.
- Um firmware em C/C++ (ou MicroPython) rodando na BitDogLab, responsável por receber e exibir os dados em um display OLED.

---

## Diagrama em Blocos do Sistema
![Diagrama em Blocos](./images/Diagrama_de_Blocos.png)

---

## Descrição dos Componentes

### FPGA (Nó Transmissor)
- Plataforma: ColorLight i9
- Base: Target colorlight_i5 do LiteX
- Processador: VexRiscv

- Periféricos:
  - SPI – comunicação com módulo LoRa RFM96
  - I2C – leitura de dados do sensor AHT10
  - Timer – leitura periódica (a cada 10 segundos)

- Funcionalidades:
  - Inicializa periféricos SPI e I2C
  - Lê temperatura e umidade via AHT10
  - Envia os dados via LoRa em formato estruturado

### BitDogLab (Nó Receptor)
- Plataforma: BitDogLab
- Periféricos:
  - SPI – módulo LoRa RFM96
  - I2C – display OLED

- Firmware: C/C++ (ou MicroPython)

- Funcionalidades:
  - Recebe pacotes via LoRa
  - Decodifica temperatura e umidade
  - Exibe os valores no display OLED
  - Atualiza a cada nova transmissão

---

## Instruções de Compilação

### BitDogLab
Para poder o código funcionar na bitdoglab, é necessário apenas, utilizando o VSCode, baixar a extensão "Raspberry PI", e importar a pasta "software" deste repositório por meio dela. Assim que finalizada a configuração, será necessário apenas enviar o código para a placa.

### FPGA
Para poder executar na FPGA, será necessário primeiro que seja instalado algumas ferramentas que sejam capazes de auxiliar na programação em hardware do FPGA, sendo elas:

- Toolchain RiscV
- OSS-CAD-SUITE
- Litex (Essa ferramenta foi a utilizada para montar o soc. Como neste repositório o soc já foi disponibilizado, não é necessário baixa-la).

Após realizada as instalações, será seguido um passo a passo:
1. Primeiro, entre no ambiente do OSS-CAD-SUITE, que terá os itens necessários para executar o programa e envia-lo para a FPGA.
```
source ./caminho/para/a/pasta/do/OSS-CAD-SUITE/environment
```

2. Já dentro do ambiente do oss-cad-suite, é possível realizar o build do soc.
```
cd hardware/litex/
python3 colorlight_i5.py --board i9 --revision 7.2 --build --cpu-type=picorv32  --ecppack-compress
```

3. Agora, será executar o comando "make" no firmware.
```
cd ../firmware/
make
```
**Caso ocorra algum erro durante o "make", tente executar o comando "make clean", e em seguida "make" novamente.**

4. Por fim, passar o soc para o FPGA, e executar o firmware que permitirá a execução do programa. Primeiro, execute o seguinte comando para descobrir onde está localizado o programa "openFPGALoader".

```
which openFPGALoader
```

E agora, será colocado o caminho descoberto na próximo comando.

```
cd ../litex/
caminho-descoberto -b colorlight-i5 litex/build/colorlight_i5/gateware/colorlight_i5.bit
```

E executando o firmware, com o seguinte comando, é possível inicializar o o programa na FPGA.

```
litex_term /dev/ttyACM0 --kernel ../firmware/main.bin
```
**Em caso de problemas no último comando, tente mudar o valor da porta (exemplo: ttyACM1), e identifique problemas de conexão.**

### Instruções de Execução

Ao executar o firmware, caso não apareça nada, pressione a tecla "enter", e abrirá o terminal, que por sua vez, deve ser digitado o comando "reboot".

Ao ser reiniciado, abrirá um menu, com diversas opções de comandos. Primeiro, deve-se executar os seguintes comandos:
```
lora_setup
sensor_setup
```

Após isso, será necessário apenas conectar a bitdoglab a uma fonte de alimentação, e executar o próximo comando:
```
sensor_send
```