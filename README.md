# IOCTL Kernel Driver Template

[![Windows](https://img.shields.io/badge/Windows-10%2B-blue.svg)](https://www.microsoft.com/windows)
[![WDK](https://img.shields.io/badge/WDK-10.0.22621.2428-green.svg)](https://docs.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Educational](https://img.shields.io/badge/Purpose-Educational-orange.svg)](https://github.com)

> **AVISO IMPORTANTE**: Este projeto é destinado **exclusivamente para fins educacionais**. O uso inadequado pode resultar em instabilidade do sistema ou violação de termos de serviço de jogos. Use apenas em ambientes controlados e com conhecimento adequado.

##  Visão Geral

Este é um template completo para desenvolvimento de drivers kernel-mode que utilizam comunicação **IOCTL** (Input/Output Control) para interação com processos de user-mode. O projeto demonstra como criar uma ponte segura entre aplicações user-mode e kernel-mode para operações de memória.

### Características Principais

- **Comunicação Kernel-Mode**: Driver rodando no espaço kernel
- **IOCTL Protocol**: Comunicação segura via DeviceIoControl
- **Bypass User-Mode**: Contorna proteções básicas de user-mode
- **Template Seguro**: Código bem estruturado e documentado
- **Educacional**: Focado em aprendizado e compreensão

## Arquitetura

### Fluxo de Comunicação

```
┌─────────────────┐    IOCTL     ┌─────────────────┐    MmCopyVirtualMemory   ┌─────────────────┐
│   User-Mode     │ ──────────►  │   Kernel-Mode   │ ───────────────────────► │   Target        │
│   Application   │              │    Driver       │                          │   Process       │
│                 │ ◄──────────  │                 │ ◄─────────────────────── │                 │
└─────────────────┘              └─────────────────┘                          └─────────────────┘
```

### Componentes Principais

1. **Kernel Driver** (`kernel_mode/`)
   - Driver kernel-mode rodando no Ring 0
   - Implementa comunicação IOCTL
   - Gerencia operações de memória

2. **User-Mode Client** (`user_mode/`)
   - Aplicação que se comunica com o driver
   - Interface para operações de memória
   - Wrapper classes para facilitar uso

3. **IOCTL Protocol**
   - Códigos de controle personalizados
   - Estruturas de dados padronizadas
   - Comunicação bidirecional

## Requisitos

### Software Necessário

- **Administrator Privileges** (para carregar drivers)
- **Test Signing** habilitado ou driver assinado


## Instalação

### 1. Preparação do Ambiente

```cmd
# Habilitar Test Signing (como Administrador)
bcdedit /set testsigning on
shutdown /r /t 0
```

### 2. Instalação do Driver

```cmd
# Copiar kernel_mode.sys para C:\teste\
# Instalar o serviço
sc create KernelDriver2024 binPath= "C:\teste\kernel_mode.sys" type= kernel

# Iniciar o driver
sc start KernelDriver2024
```

## Como Usar

### 1. Iniciar o Driver

```cmd
# Verificar se está rodando
sc query KernelDriver2024

# Se necessário, iniciar
sc start KernelDriver2024
```

### 2. Executar Aplicação User-Mode

```cmd
# Executar como Administrador
user_mode.exe
```

### 3. Exemplo de Uso

```cpp
#include "driver_helper.h"

int main() {
    // Conectar com o driver
    ioctl::Driver driver;
    
    // Attach ao processo
    Process process("target.exe");
    driver.attach_to_process(process.pid);
    
    // Ler memória
    int value = driver.read_memory<int>(0x401000);
    
    // Escrever memória
    driver.write_memory<int>(0x401000, 999);
    
    return 0;
}
```

## Comunicação IOCTL

### Códigos de Controle

```cpp
namespace codes {
    constexpr ULONG attach = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x696, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
    constexpr ULONG read   = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x697, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
    constexpr ULONG write  = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x698, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
}
```

### Estrutura de Requisição

```cpp
struct Request {
    HANDLE process_id;    // PID do processo alvo
    PVOID target;         // Endereço de memória
    PVOID buffer;         // Buffer de dados
    SIZE_T size;          // Tamanho dos dados
    SIZE_T return_size;   // Tamanho retornado
};
```

### Fluxo de Operação

1. **User-Mode** cria requisição IOCTL
2. **Kernel Driver** recebe via `device_control`
3. **Driver** executa operação (`MmCopyVirtualMemory`)
4. **Resultado** retornado para User-Mode

## Estrutura do Projeto

```
IOCTL-Kernel-Cheat-Driver-Template/
├── kernel_mode/                 # Driver Kernel-Mode
│   ├── source/
│   │   └── main.cpp            # Código principal do driver
│   ├── kernel_mode.inf         # Arquivo de instalação
│   └── kernel_mode.vcxproj     # Projeto Visual Studio
├── user_mode/                   # Aplicação User-Mode
│   ├── headers/
│   │   ├── driver_helper.h     # Wrapper para comunicação IOCTL
│   │   ├── process_helper.h    # Gerenciamento de processos
│   │   ├── error_helper.h      # Tratamento de erros
│   │   └── offsets.h           # Offsets de memória
│   ├── source/
│   │   ├── driver_helper.cpp   # Implementação do wrapper
│   │   ├── process_helper.cpp  # Implementação de processos
│   │   ├── error_helper.cpp    # Implementação de erros
│   │   └── main.cpp            # Aplicação principal
│   └── user_mode.vcxproj       # Projeto Visual Studio
└── README.md                    # Este arquivo
```

## Exemplos Práticos

### Exemplo 1: Leitura de Memória

```cpp
// Conectar com driver
ioctl::Driver driver;
Process process("notepad.exe");

// Attach ao processo
driver.attach_to_process(process.pid);

// Ler valor de 32 bits
int value = driver.read_memory<int>(0x401000);
std::cout << "Valor lido: " << value << std::endl;
```

### Exemplo 2: Escrita de Memória

```cpp
// Escrever novo valor
int newValue = 999;
driver.write_memory<int>(0x401000, newValue);

// Verificar se foi escrito
int verifyValue = driver.read_memory<int>(0x401000);
if (verifyValue == newValue) {
    std::cout << "Escrita bem-sucedida!" << std::endl;
}
```

### Exemplo 3: Menu Interativo

```cpp
void printMenu() {
    std::cout << "\n=== IOCTL DRIVER TEST MENU ===" << std::endl;
    std::cout << "1. Ler informações do processo" << std::endl;
    std::cout << "2. Ler memória específica" << std::endl;
    std::cout << "3. Escrever memória específica" << std::endl;
    std::cout << "4. Sair" << std::endl;
}
```

## Debugging

### DebugView

Para visualizar mensagens do driver:

1. **Baixar DebugView** da Microsoft
2. **Executar como Administrador**
3. **Configurar filtros:**
   ```
   Include: KernelDriver2024
   Capture Kernel: ✅
   ```
4. **Reiniciar driver** para ver mensagens de inicialização

### Mensagens de Debug

```cpp
// No driver kernel
DbgPrint("[+] Driver Kernel-Mode IOCTL carregado.\n");
DbgPrint("[+] Device do driver criado com sucesso.\n");
DbgPrint("[+] Symbolic Link estabelecido com sucesso.\n");
DbgPrint("[+] Driver inicializado com sucesso.\n");
DbgPrint("[+] Controle de dispositivo chamado.\n");
DbgPrint("[+] Descarregamento do driver chamado. Limpando...\n");
DbgPrint("[+] Driver descarregado com sucesso.\n");
```

### Logs Esperados

```
[+] Driver Kernel-Mode IOCTL carregado.
[+] Device do driver criado com sucesso.
[+] Symbolic Link estabelecido com sucesso.
[+] Driver inicializado com sucesso.
[+] Controle de dispositivo chamado.
```

## Limitações

### Segurança

- **Não funciona** em sistemas com Secure Boot
- **Requer** privilégios de Administrador
- **Pode causar** BSOD se mal implementado


## Conceitos Técnicos

### IOCTL (Input/Output Control)

IOCTL é um mecanismo do Windows para comunicação entre user-mode e kernel-mode através de códigos de controle personalizados.

### MmCopyVirtualMemory

Função kernel que permite copiar memória entre processos de forma segura, contornando proteções básicas de user-mode.

### Symbolic Links

Links simbólicos que permitem acesso ao device object a partir do user-mode (ex: `\\.\KernelDriver2024`).

##  Recursos Educacionais

### Links Úteis

- [Windows Driver Kit Documentation](https://docs.microsoft.com/en-us/windows-hardware/drivers/)
- [IOCTL Reference](https://docs.microsoft.com/en-us/windows-hardware/drivers/kernel/defining-i-o-control-codes)
- [Kernel-Mode Programming](https://docs.microsoft.com/en-us/windows-hardware/drivers/kernel/)
