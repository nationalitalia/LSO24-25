import javax.swing.*;
import java.awt.*;
import java.io.*;
import java.net.*;
import java.util.ArrayList;
import java.util.List;

public class TrisClient {
    private Socket socket;
    private BufferedReader in;
    private PrintWriter out;
    private JFrame frame = new JFrame("Tris Online");
    private JTextArea messageArea = new JTextArea(10, 40);
    private JButton[] buttons = new JButton[9];
    private String username;

    public TrisClient(String serverAddress, int port, String username) {
        this.username = username;
        try {
            // UNA SOLA CONNESSIONE
            socket = new Socket(serverAddress, port);
            in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
            out = new PrintWriter(socket.getOutputStream(), true);

            // Setup Interfaccia
            setupGUI();

            // Invio nome e avvio ascolto
            out.println("NOME " + this.username);
            new Thread(this::listenToServer).start();

        } catch (IOException e) {
            e.printStackTrace();
            JOptionPane.showMessageDialog(null, "Errore di connessione al server.");
        }
    }

    private void setupGUI() {
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setTitle("Tris Online - " + username); // Mostra il nome nella barra
        messageArea.setEditable(false);
        frame.add(new JScrollPane(messageArea), BorderLayout.SOUTH);

        JPanel boardPanel = new JPanel(new GridLayout(3, 3));
        for (int i = 0; i < 9; i++) {
            final int pos = i + 1;
            buttons[i] = new JButton("");
            buttons[i].setFont(new Font("Arial", Font.BOLD, 40));
            buttons[i].addActionListener(e -> out.println(String.valueOf(pos)));
            boardPanel.add(buttons[i]);
        }

        JPanel controlPanel = new JPanel();
        JButton btnCreate = new JButton("CREATE");
        btnCreate.addActionListener(e -> out.println("CREATE"));
        JButton btnList = new JButton("LIST");
        btnList.addActionListener(e -> out.println("LIST"));
        JButton btnYes = new JButton("SI");
        btnYes.addActionListener(e -> out.println("SI"));
        JButton btnNo = new JButton("NO");
        btnNo.addActionListener(e -> out.println("NO"));

        controlPanel.add(btnCreate);
        controlPanel.add(btnList);
        controlPanel.add(btnYes);
        controlPanel.add(btnNo);

        frame.add(boardPanel, BorderLayout.CENTER);
        frame.add(controlPanel, BorderLayout.NORTH);
        frame.pack();
        frame.setLocationRelativeTo(null);
        frame.setVisible(true);
    }

    private void listenToServer() {
        try {
            String response;
            List<String> tempGameList = new ArrayList<>();
            boolean receivingList = false;

            while ((response = in.readLine()) != null) {
                System.out.println("DEBUG RICEVUTO: " + response);
                String msg = response.trim();

                if (msg.equals("LIST_START")) {
                    receivingList = true;
                    tempGameList.clear();
                }
                else if (msg.equals("LIST_END")) {
                    receivingList = false;
                    List<String> listToDisplay = new ArrayList<>(tempGameList);
                    showGameList(listToDisplay);
                }
                else if (receivingList && msg.startsWith("GAME:")) {
                    tempGameList.add(msg.substring(5));
                }
                else {
                    SwingUtilities.invokeLater(() -> {
                        messageArea.append(msg + "\n");
                        if (msg.contains("disconnesso")) {
                            JOptionPane.showMessageDialog(frame, msg, "Notifica", JOptionPane.INFORMATION_MESSAGE);
                        }
                    });
                }
            }
        } catch (IOException e) {
            SwingUtilities.invokeLater(() -> messageArea.append("Connessione persa.\n"));
        }
    }

    private void showGameList(List<String> gameIds) {
        SwingUtilities.invokeLater(() -> {
            JDialog listDialog = new JDialog(frame, "Partite Disponibili:", true);
            listDialog.setLayout(new BorderLayout());

            DefaultListModel<String> listModel = new DefaultListModel<>();
            for (String id : gameIds) listModel.addElement(id);

            JList<String> jList = new JList<>(listModel);
            JButton joinBtn = new JButton("ENTRA");

            joinBtn.addActionListener(e -> {
                String selected = jList.getSelectedValue();
                if (selected != null) {
                    String id = selected.split(" ")[0];
                    out.println("JOIN " + id);
                    listDialog.dispose();
                }
            });

            listDialog.add(new JLabel(" Seleziona una sfida:"), BorderLayout.NORTH);
            listDialog.add(new JScrollPane(jList), BorderLayout.CENTER);
            listDialog.add(joinBtn, BorderLayout.SOUTH);
            listDialog.setSize(300, 300);
            listDialog.setLocationRelativeTo(frame);
            listDialog.setVisible(true);
        });
    }

    public static void main(String[] args) {
        String name = JOptionPane.showInputDialog(null, "Inserisci il tuo nome utente:", "Login", JOptionPane.QUESTION_MESSAGE);
        if (name != null && !name.trim().isEmpty()) {
            new TrisClient("127.0.0.1", 9100, name.trim());
        } else {
            System.exit(0);
        }
    }
}