#ifndef CLASS_LOADER_HPP
#define CLASS_LOADER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "estrutura_dados.hpp"
#include "runtime.hpp"

/**
 * Carrega arquivos .class sob demanda e os mantém em memória (Method Area).
 *
 * Responsabilidades:
 *  - Receber um nome de classe (ex: "Hello", "java/lang/Object")
 *  - Localizar o arquivo .class correspondente no classpath
 *  - Usar o Parser para transformar os bytes em class_info
 *  - Cachear o resultado — a mesma classe nunca é parseada duas vezes
 *  - Expor métodos para o interpretador buscar classes, métodos e campos estáticos
 */
class ClassLoader {
public:
    /**
     * @param classpath Diretórios onde procurar arquivos .class, em ordem de prioridade.
     *                  Se vazio, usa apenas o diretório atual (".").
     */
    explicit ClassLoader(std::vector<std::string> classpath = {"."});

    /**
     * Carrega a classe pelo nome interno da JVM (barras, não pontos).
     * Exemplos: "Hello", "exemplos/fatorial", "java/lang/String"
     *
     * - Se já estiver carregada, retorna a referência cacheada.
     * - Se não estiver, localiza o .class, parseia, armazena e retorna.
     * - Lança std::runtime_error se o arquivo não for encontrado.
     *
     * A referência retornada é estável enquanto o ClassLoader existir.
     */
    const class_info& load(const std::string& class_name);

    /**
     * Retorna true se a classe já está carregada (sem tentar carregar).
     */
    bool isLoaded(const std::string& class_name) const;

    // Controle de inicialização — <clinit> é executado pelo interpretador,
    // mas o ClassLoader rastreia quais classes já foram inicializadas.
    bool isInitialized(const std::string& class_name) const;
    void markInitialized(const std::string& class_name);

    // -------------------------------------------------------------------------
    // Campos estáticos (parte da Method Area)
    // Chave: "NomeDaClasse::nomeDoCampo"  ex: "ContadorGlobal::total"
    // -------------------------------------------------------------------------

    bool  hasStaticField(const std::string& key) const;
    void  setStaticField(const std::string& key, Value value);
    Value getStaticField(const std::string& key) const;

private:
    std::vector<std::string> classpath_;

    // A Method Area: nome interno da classe → class_info parseada
    std::unordered_map<std::string, class_info> classes_;

    // Campos estáticos: "Classe::campo" → valor (Value suporta todos os tipos JVM)
    std::unordered_map<std::string, Value> static_fields_;

    // Classes que já tiveram <clinit> executado pelo interpretador
    std::unordered_set<std::string> initialized_classes_;

    /**
     * Transforma o nome interno da classe em caminho de arquivo.
     * "Hello"              → "Hello.class"
     * "exemplos/fatorial"  → "exemplos/fatorial.class"
     */
    std::string resolvePath(const std::string& class_name) const;
};

#endif // CLASS_LOADER_HPP
