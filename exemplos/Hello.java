public class Hello {

    static int parametroEstatico;

    int parametroInstancia;

    public static void main(String[] args) {
        System.out.println("Hello, World!");
    }

    void print() {
        System.out.println("Parametro de instancia = " + parametroInstancia);
        System.out.println("Parametro estatico = " + parametroEstatico);
    }
    
}