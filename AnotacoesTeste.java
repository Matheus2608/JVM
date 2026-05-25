import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

@Retention(RetentionPolicy.RUNTIME)
@interface MinhaAnotacao {
    String valor() default "teste";
}

public class AnotacoesTeste {
    @MinhaAnotacao(valor = "Implementado")
    public int campoComAnotacao;
    
    @Deprecated
    public String campoObsoleto;
}