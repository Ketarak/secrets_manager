let port = null;

// Établit la connexion avec le binaire C
function connectToNativeHost() {
  if (port) return port;
  
  console.log("Connecting to native host 'passmgr'...");
  port = browser.runtime.connectNative("passmgr");
  
  // Écoute les messages retournés par le binaire C
  port.onMessage.addListener((response) => {
    console.log("Received response from native host:", response);
    // Relaye la réponse à la Popup (si elle est ouverte)
    browser.runtime.sendMessage({ target: "popup", data: response }).catch(err => {
      // Ignore l'erreur si la popup est fermée
    });
  });
  
  // Gère la déconnexion inattendue ou fermeture du binaire
  port.onDisconnect.addListener((p) => {
    console.log("Disconnected from native host. Error:", p.error);
    port = null;
    // Notifie la popup de la perte de connexion
    browser.runtime.sendMessage({ 
      target: "popup", 
      data: { status: "disconnected", error: p.error?.message } 
    }).catch(err => {});
  });
  
  return port;
}

// Écoute les requêtes en provenance de notre Popup JS
browser.runtime.onMessage.addListener((message, sender, sendResponse) => {
  if (message.target === "background") {
    const action = message.action;
    
    // S'assure que la connexion avec l'hôte C est active
    const activePort = connectToNativeHost();
    
    if (action === "sendToHost") {
      try {
        console.log("Sending payload to native host:", message.payload);
        activePort.postMessage(message.payload);
      } catch (err) {
        console.error("Failed to send message to native host:", err);
        sendResponse({ status: "error", message: "Failed to communicate with native app" });
      }
    }
  }
  return true; // Garde le canal ouvert pour les réponses asynchrones
});

// Première connexion au lancement
connectToNativeHost();