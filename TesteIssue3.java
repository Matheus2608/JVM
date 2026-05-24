import java.io.IOException;
import java.util.List;

public class TesteIssue3 {
    // Este método vai forçar a JVM a gerar 3 dos 4 atributos novos!
    @Deprecated
    public void testarAtributos(List<String> lista) throws IOException, RuntimeException {
        int variavelQualquer = 42;
    }
}