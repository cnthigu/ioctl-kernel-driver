#include <iostream>
#include <cstdint>
#include <map>
#include <string>

#include <Windows.h>
#include <psapi.h>
#include <TlHelp32.h>

#include "../headers/process_helper.h"
#include "../headers/driver_helper.h"
#include "../headers/error_helper.h"
#include "../headers/offsets.h"

void printMenu() {
    std::cout << "\n=== IOCTL DRIVER TEST MENU ===\n";
    std::cout << "1. Ler informacoes do processo\n";
    std::cout << "2. Ler memoria especifica\n";
    std::cout << "3. Escrever memoria especifica\n";
    std::cout << "4. Teste com Notepad (ler/escrever texto)\n";
    std::cout << "5. Teste com AssaultCube (modificar balas)\n";
    std::cout << "6. Sair\n";
    std::cout << "Escolha uma opcao: ";
}

int main()
{
    const char* process_name = "ac_client.exe"; 
    Process process(process_name);

    ioctl::Driver driver;

    if (process.handle == INVALID_HANDLE_VALUE)
    {
        std::string errorMsg = GetErrorString(GetLastError());
        std::cout << "[-] Failed to create process handle: " << errorMsg;
        std::cout << "\n[!] Certifique-se de que o " << process_name << " esta aberto!\n";
        std::cin.get();
        return 1;
    }

    std::cout << "[+] Processo encontrado! PID: " << process.pid << "\n";

    {
        std::cout << "\n--------------------------------------\n";
        std::cout << "Modules loaded into " << process_name << ": \n";
        process.PrintAllModules(true);
        std::cout << "--------------------------------------\n\n";
    }

    if (driver.driver_handle == INVALID_HANDLE_VALUE)
    {
        std::string errorMsg = GetErrorString(GetLastError());
        std::cout << "[-] Failed to create driver handle: " << errorMsg << '\n';
        std::cout << "[!] Certifique-se de que o driver KernelDriver2024 esta rodando!\n";
        std::cin.get();
        return 1;
    }

    std::cout << "[+] Driver conectado com sucesso!\n";

    if (!driver.attach_to_process(process.pid))
    {
        std::cout << "[-] Falha ao fazer attach ao processo via driver!\n";
        std::cin.get();
        return 1;
    }

    std::cout << "[+] Attach ao processo realizado com sucesso!\n";

    uintptr_t baseAddr = process.modules["ac_client.exe"];
    std::cout << "[+] Base address: 0x" << std::hex << baseAddr << std::dec << "\n";

    int choice;
    do {
        printMenu();
        std::cin >> choice;

        switch (choice) {
        case 1: {
            std::cout << "\n=== INFORMACOES DO PROCESSO ===\n";
            std::cout << "[+] PID: " << process.pid << "\n";
            std::cout << "[+] Handle: 0x" << std::hex << process.handle << std::dec << "\n";
            std::cout << "[+] Numero de modulos: " << process.modules.size() << "\n";
            std::cout << "[+] Base address: 0x" << std::hex << baseAddr << std::dec << "\n";

            std::cout << "\n[+] Modulos principais:\n";
            for (const auto& [name, addr] : process.modules) {
                if (name.find("notepad") != std::string::npos ||
                    name.find("kernel32") != std::string::npos ||
                    name.find("user32") != std::string::npos) {
                    std::cout << "    " << name << " -> 0x" << std::hex << addr << std::dec << "\n";
                }
            }
            break;
        }

        case 2: {
            std::cout << "\n=== LEITURA DE MEMORIA ESPECIFICA ===\n";
            std::cout << "[+] Digite um endereco em hexadecimal (ex: 0x401000): ";

            std::string addrStr;
            std::cin >> addrStr;

            try {
                uintptr_t addr = std::stoull(addrStr, nullptr, 16);

                // Lê diferentes tipos de dados
                uint8_t byteVal = driver.read_memory<uint8_t>(addr);
                uint16_t wordVal = driver.read_memory<uint16_t>(addr);
                uint32_t dwordVal = driver.read_memory<uint32_t>(addr);
                uint64_t qwordVal = driver.read_memory<uint64_t>(addr);

                std::cout << "[+] Valores lidos do endereco 0x" << std::hex << addr << std::dec << ":\n";
                std::cout << "    BYTE  (8 bits):  0x" << std::hex << (int)byteVal << std::dec << "\n";
                std::cout << "    WORD  (16 bits): 0x" << std::hex << wordVal << std::dec << "\n";
                std::cout << "    DWORD (32 bits): 0x" << std::hex << dwordVal << std::dec << "\n";
                std::cout << "    QWORD (64 bits): 0x" << std::hex << qwordVal << std::dec << "\n";

            }
            catch (...) {
                std::cout << "[-] Erro ao ler memoria ou endereco invalido\n";
            }
            break;
        }

        case 3: {
            std::cout << "\n=== ESCRITA DE MEMORIA ESPECIFICA ===\n";
            std::cout << "[+] Digite um endereco em hexadecimal (ex: 0x401000): ";

            std::string addrStr;
            std::cin >> addrStr;

            try {
                uintptr_t addr = std::stoull(addrStr, nullptr, 16);

                std::cout << "[+] Digite o valor para escrever: ";
                int value;
                std::cin >> value;

                // Escrever valor
                driver.write_memory<int>(addr, value);

                // Verificar se foi escrito
                int verifyValue = driver.read_memory<int>(addr);
                std::cout << "[+] Valor escrito: " << verifyValue << "\n";

            }
            catch (...) {
                std::cout << "[-] Erro ao escrever memoria ou endereco invalido\n";
            }
            break;
        }
        case 5: {
            std::cout << "\n=== TESTE COM ASSAULTCUBE ===\n";
            std::cout << "[!] Para testar com AssaultCube:\n";
            std::cout << "    1. Mude o process_name para 'ac_client.exe'\n";
            std::cout << "    2. Recompile o programa\n";
            std::cout << "    3. Abra o AssaultCube\n";
            std::cout << "    4. Use Cheat Engine para encontrar o endereco das balas\n";
            std::cout << "    5. Use a opção 2 ou 3 com o endereco das balas\n";

            std::cout << "\n[+] Endereco exemplo das balas: baseAddr + 0x10F4F4\n";
            std::cout << "[+] (Use Cheat Engine para encontrar o endereco correto)\n";
            break;
        }

        case 6:
            std::cout << "\n[+] Saindo...\n";
            break;

        default:
            std::cout << "\n[-] Opcao invalida!\n";
            break;
        }

        if (choice != 6) {
            std::cout << "\nPressione ENTER para continuar...";
            std::cin.ignore();
            std::cin.get();
        }

    } while (choice != 6);

    std::cout << "\n[+] Programa finalizado!\n";
    std::cin.get();
    return 0;
}