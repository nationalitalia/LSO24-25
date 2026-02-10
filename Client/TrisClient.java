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
    private String lastChallenger = "";
    private JLabel vsLabel = new JLabel("In attesa...", SwingConstants.CENTER);

    public TrisClient(String serverAddress, int port, String username) {
        this.username = username;
        try {
            socket = new Socket(serverAddress, port);
            in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
            out = new PrintWriter(socket.getOutputStream(), true);

            setupGUI();

            out.println("NOME " + this.username);
            new Thread(this::listenToServer).start();

        } catch (IOException e) {
            e.printStackTrace();
            JOptionPane.showMessageDialog(null, "Errore di connessione al server.");
        }
    }

    private void setupGUI() {
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setTitle("Tris Online - " + username);
        frame.setLayout(new BorderLayout(15, 15));
        frame.getContentPane().setBackground(new Color(245, 245, 245));

        // Barra Superiore
        JPanel northPanel = new JPanel(new BorderLayout());
        northPanel.setOpaque(false);
        northPanel.setBorder(BorderFactory.createEmptyBorder(10, 20, 0, 20));

        JLabel titleLabel = new JLabel("TRIS ONLINE");
        titleLabel.setFont(new Font("SansSerif", Font.BOLD, 22));
        titleLabel.setForeground(new Color(50, 50, 50));

        JPanel managementPanel = new JPanel(new FlowLayout(FlowLayout.RIGHT, 10, 0));
        managementPanel.setOpaque(false);

        JButton btnCreate = createStyledButton("CREATE", new Color(40, 167, 69));
        JButton btnList = createStyledButton("LIST", new Color(0, 123, 255));
        JButton btnDelete = createStyledButton("DELETE", new Color(220, 53, 69));

        btnCreate.addActionListener(e -> out.println("CREATE"));
        btnList.addActionListener(e -> out.println("LIST"));
        btnDelete.addActionListener(e -> out.println("DELETE"));

        managementPanel.add(btnCreate);
        managementPanel.add(btnList);
        managementPanel.add(btnDelete);

        northPanel.add(titleLabel, BorderLayout.WEST);
        northPanel.add(managementPanel, BorderLayout.EAST);
        frame.add(northPanel, BorderLayout.NORTH);

        // Parte Centrale (Griglia)
        JPanel centerPanel = new JPanel(new GridBagLayout());
        centerPanel.setOpaque(false);

        JPanel boardPanel = new JPanel(new GridLayout(3, 3, 8, 8));
        boardPanel.setPreferredSize(new Dimension(350, 350));
        boardPanel.setOpaque(false);

        for (int i = 0; i < 9; i++) {
            final int pos = i + 1;
            buttons[i] = new JButton("");
            buttons[i].setFont(new Font("Arial", Font.BOLD, 45));
            buttons[i].setFocusPainted(false);
            buttons[i].setBackground(Color.WHITE);
            buttons[i].setBorder(BorderFactory.createLineBorder(new Color(200, 200, 200), 2));
            buttons[i].addActionListener(e -> out.println(String.valueOf(pos)));
            boardPanel.add(buttons[i]);
        }

        centerPanel.add(boardPanel);
        frame.add(centerPanel, BorderLayout.CENTER);

        // Parte inferiore (Log e Decisioni)
        JPanel southPanel = new JPanel(new BorderLayout(15, 0));
        southPanel.setBorder(BorderFactory.createEmptyBorder(0, 20, 15, 20));
        southPanel.setOpaque(false);

        messageArea.setEditable(false);
        messageArea.setFont(new Font("Monospaced", Font.PLAIN, 12));
        messageArea.setBackground(new Color(40, 44, 52));
        messageArea.setForeground(new Color(171, 178, 191));
        JScrollPane scrollPane = new JScrollPane(messageArea);
        scrollPane.setPreferredSize(new Dimension(300, 130));
        scrollPane.setBorder(BorderFactory.createLineBorder(new Color(60, 60, 60)));

        JPanel decisionPanel = new JPanel();
        decisionPanel.setLayout(new BoxLayout(decisionPanel, BoxLayout.Y_AXIS));
        decisionPanel.setOpaque(false);
        decisionPanel.setPreferredSize(new Dimension(180, 130));

        JPanel vsWrapper = new JPanel(new BorderLayout());
        vsWrapper.setOpaque(false);
        vsWrapper.setBorder(BorderFactory.createTitledBorder("PARTITA"));

        vsLabel.setFont(new Font("SansSerif", Font.BOLD, 12));
        vsLabel.setHorizontalAlignment(SwingConstants.CENTER);
        vsLabel.setText("<html><center>In attesa<br>di sfidanti</center></html>");
        vsWrapper.add(vsLabel, BorderLayout.CENTER);

        JButton btnLeave = createStyledButton("LEAVE (Esci)", new Color(255, 140, 0));
        btnLeave.setAlignmentX(Component.CENTER_ALIGNMENT);
        btnLeave.setMaximumSize(new Dimension(160, 35));

        btnLeave.addActionListener(e -> {
            int confirm = JOptionPane.showConfirmDialog(frame, "Vuoi davvero abbandonare?", "Conferma", JOptionPane.YES_NO_OPTION);
            if (confirm == JOptionPane.YES_OPTION) {
                out.println("LEAVE");
                resetBoard();
            }
        });

        decisionPanel.add(Box.createVerticalGlue());
        decisionPanel.add(vsWrapper);
        decisionPanel.add(Box.createVerticalStrut(10));
        decisionPanel.add(btnLeave);
        decisionPanel.add(Box.createVerticalGlue());

        southPanel.add(scrollPane, BorderLayout.CENTER);
        southPanel.add(decisionPanel, BorderLayout.EAST);
        frame.add(southPanel, BorderLayout.SOUTH);

        frame.setMinimumSize(new Dimension(650, 680));
        frame.pack();
        frame.setLocationRelativeTo(null);
        frame.setVisible(true);
    }

    private JButton createStyledButton(String text, Color bg) {
        JButton btn = new JButton(text);
        btn.setPreferredSize(new Dimension(110, 35));
        btn.setBackground(bg);
        btn.setForeground(Color.WHITE);
        btn.setFocusPainted(false);
        btn.setFont(new Font("SansSerif", Font.BOLD, 12));
        btn.setBorder(BorderFactory.createEmptyBorder());
        btn.setCursor(new Cursor(Cursor.HAND_CURSOR));
        return btn;
    }

    private void listenToServer() {
        try {
            String response;
            List<String> tempGameList = new ArrayList<>();
            List<String> tempDeleteList = new ArrayList<>();
            boolean receivingList = false;
            boolean receivingDeleteList = false;
            int rowCount = 0;

            while ((response = in.readLine()) != null) {
                String msg = response.trim();
                if (msg.isEmpty()) continue;

                // --- GESTIONE LISTE (Invariata) ---
                if (msg.equals("LIST_START")) { receivingList = true; tempGameList.clear(); }
                else if (msg.equals("LIST_END")) { receivingList = false; showGameList(new ArrayList<>(tempGameList)); }
                else if (receivingList && msg.startsWith("GAME:")) { tempGameList.add(msg.substring(5)); }
                else if (msg.equals("LIST_DELETE_START")) { receivingDeleteList = true; tempDeleteList.clear(); }
                else if (msg.equals("LIST_DELETE_END")) { receivingDeleteList = false; showDeleteGameList(new ArrayList<>(tempDeleteList)); }
                else if (receivingDeleteList && msg.startsWith("OWNGAME:")) { tempDeleteList.add(msg.substring(7)); }

                // --- SINCRONIZZAZIONE GRIGLIA ---
                else if (msg.contains("|")) {
                    updateButtonsFromRow(msg, rowCount);
                    rowCount++;
                    if (rowCount > 2) rowCount = 0;
                }
                else if (msg.contains("Board:") || msg.contains("campo")) {
                    rowCount = 0;
                }

                // --- TUTTA LA LOGICA DEI MESSAGGI VA QUI DENTRO ---
                else {
                    SwingUtilities.invokeLater(() -> {
                        // 1. Comando speciale per i nomi
                        if (msg.startsWith("OPPONENT_IS")) {
                            String oppName = msg.split(" ")[1].trim();
                            vsLabel.setText("<html><center><b style='color:blue;'>" + username + "</b><br>" +
                                            "<span style='font-size:9px; color:gray;'>VS</span><br>" +
                                            "<b style='color:red;'>" + oppName + "</b></center></html>");
                        } 
                        
                        // 2. FINE PARTITA (Questo ferma tutto finché non premi OK)
                        else if (msg.contains("vinto") || msg.contains("perso") || msg.contains("Pareggio")) {
                            messageArea.append(msg + "\n");
                            new Thread(() -> {
                                // 1. Mostra il risultato (Blocca questo thread finché non premi OK)
                                JOptionPane.showMessageDialog(frame, msg, "Fine Partita", JOptionPane.INFORMATION_MESSAGE);
                                
                                // 2. Una volta premuto OK, resettiamo la board
                                SwingUtilities.invokeLater(() -> resetBoard());
                            }).start();
                        }

                        // 3. RIVINCITA (Apparirà solo dopo la chiusura del messaggio sopra)
                        else if (msg.startsWith("RIVINCITA_DOMANDA")) {
                            Timer delayRivincita = new Timer(2000, e -> {
                                int scelta = JOptionPane.showConfirmDialog(frame, 
                                    "Vuoi la rivincita?", "Rivincita", JOptionPane.YES_NO_OPTION);
                                
                                if (scelta == JOptionPane.YES_OPTION) {
                                    out.println("RIVINCITA_SI");
                                } else {
                                    out.println("RIVINCITA_NO");
                                    resetBoard(); 
                                }
                            });
                            delayRivincita.setRepeats(false);
                            delayRivincita.start();
                        }

                        // 4. ALTRI MESSAGGI
                        else {
                            messageArea.append(msg + "\n");
                            if (msg.contains("vuole unirsi alla tua partita")) {
                                String nomeS = "Sfidante";
                                try { nomeS = msg.split(" ")[2].trim(); } catch (Exception e) {}
                                final String sfidanteFinal = nomeS;
                                int scelta = JOptionPane.showConfirmDialog(frame, msg, "Sfida", JOptionPane.YES_NO_OPTION);
                                out.println((scelta == JOptionPane.YES_OPTION ? "ACCETTA " : "RIFIUTA ") + sfidanteFinal);
                            }
                            if (msg.contains("disconnesso")) {
                                JOptionPane.showMessageDialog(frame, msg, "Notifica", JOptionPane.WARNING_MESSAGE);
                                resetBoard();
                            }
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
            JDialog listDialog = new JDialog(frame, "Partite Disponibili", true);
            listDialog.setLayout(new BorderLayout());
            DefaultListModel<String> listModel = new DefaultListModel<>();
            for (String id : gameIds) listModel.addElement(id);
            JList<String> jList = new JList<>(listModel);
            JButton joinBtn = new JButton("ENTRA");
            joinBtn.addActionListener(e -> {
                String selected = jList.getSelectedValue();
                if (selected != null) {
                    out.println("JOIN " + selected.split(" ")[0]);
                    listDialog.dispose();
                }
            });
            listDialog.add(new JScrollPane(jList), BorderLayout.CENTER);
            listDialog.add(joinBtn, BorderLayout.SOUTH);
            listDialog.setSize(300, 300);
            listDialog.setLocationRelativeTo(frame);
            listDialog.setVisible(true);
        });
    }

    private void showDeleteGameList(List<String> gameIds) {
        SwingUtilities.invokeLater(() -> {
            JDialog listDialog = new JDialog(frame, "Le tue Partite", true);
            listDialog.setLayout(new BorderLayout());
            DefaultListModel<String> listModel = new DefaultListModel<>();
            for (String id : gameIds) listModel.addElement(id);
            JList<String> jList = new JList<>(listModel);
            JButton delBtn = new JButton("ELIMINA");
            delBtn.addActionListener(e -> {
                String selected = jList.getSelectedValue();
                if (selected != null) {
                    String id = selected.split(" ")[0].replaceAll("[^0-9]", "");
                    out.println("DELETE " + id);
                    listDialog.dispose();
                }
            });
            listDialog.add(new JScrollPane(jList), BorderLayout.CENTER);
            listDialog.add(delBtn, BorderLayout.SOUTH);
            listDialog.setSize(300, 300);
            listDialog.setLocationRelativeTo(frame);
            listDialog.setVisible(true);
        });
    }

    private void updateButtonsFromRow(String rowText, int rowIndex) {
        SwingUtilities.invokeLater(() -> {
            try {
                String[] parts = rowText.split("\\|");
                for(int i = 0; i < parts.length && i < 3; i++) {
                    int buttonIndex = (rowIndex * 3) + i;
                    String val = parts[i].trim();
                    if (!val.isEmpty()) {
                        buttons[buttonIndex].setText(val);
                        buttons[buttonIndex].setForeground(val.equalsIgnoreCase("X") ? Color.RED : Color.BLUE);
                    } else {
                        buttons[buttonIndex].setText("");
                    }
                }
            } catch (Exception e) {}
        });
    }

    private void resetBoard() {
        for (JButton b : buttons) b.setText("");
        vsLabel.setText("<html><center>In attesa<br>di sfidanti</center></html>");
    }

    public static void main(String[] args) {
        String name = JOptionPane.showInputDialog(null, "Nome utente:", "Login", JOptionPane.QUESTION_MESSAGE);
        if (name != null && !name.trim().isEmpty()) {
            new TrisClient("127.0.0.1", 9100, name.trim());
        } else {
            System.exit(0);
        }
    }
}