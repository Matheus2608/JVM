#include "../loader/class_loader.hpp"
#include "../parser/parser.hpp"
#include "cp_utils.hpp"

#include <stdexcept>
#include <fstream>

ClassLoader::ClassLoader(std::vector<std::string> classpath)
    : classpath_(std::move(classpath)) {}

// -----------------------------------------------------------------------------
// Carregamento
// -----------------------------------------------------------------------------

const class_info& ClassLoader::load(const std::string& class_name) {
    // Cache hit — não reparseia
    auto it = classes_.find(class_name);
    if (it != classes_.end())
        return it->second;

    // Localiza o arquivo .class no classpath
    std::string filepath = resolvePath(class_name);
    if (filepath.empty())
        throw std::runtime_error("ClassLoader: classe nao encontrada: " + class_name);

    // Parseia e armazena
    Parser parser(filepath);
    classes_[class_name] = parser.parse();

    return classes_[class_name];
}

bool ClassLoader::isLoaded(const std::string& class_name) const {
    return classes_.count(class_name) > 0;
}

// -----------------------------------------------------------------------------
// Inicialização de classes
// -----------------------------------------------------------------------------

bool ClassLoader::isInitialized(const std::string& class_name) const {
    return initialized_classes_.count(class_name) > 0;
}

void ClassLoader::markInitialized(const std::string& class_name) {
    initialized_classes_.insert(class_name);
}

// -----------------------------------------------------------------------------
// Campos estáticos
// -----------------------------------------------------------------------------

bool ClassLoader::hasStaticField(const std::string& key) const {
    return static_fields_.count(key) > 0;
}

void ClassLoader::setStaticField(const std::string& key, Value value) {
    static_fields_[key] = value;
}

Value ClassLoader::getStaticField(const std::string& key) const {
    auto it = static_fields_.find(key);
    if (it == static_fields_.end())
        throw std::runtime_error("ClassLoader: campo estatico nao inicializado: " + key);
    return it->second;
}

// -----------------------------------------------------------------------------
// Resolução de caminho
// -----------------------------------------------------------------------------

std::string ClassLoader::resolvePath(const std::string& class_name) const {
    // class_name já usa barras: "java/lang/String" → "java/lang/String.class"
    std::string relative = class_name + ".class";

    for (const std::string& dir : classpath_) {
        std::string full = dir + "/" + relative;
        // Checagem de existência em C++11: tenta abrir o arquivo.
        std::ifstream f(full.c_str());
        if (f.good())
            return full;
    }
    return "";
}
