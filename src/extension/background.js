let port = null;
let nativeRequestQueue = [];

// Établit la connexion avec le binaire C
function connectToNativeHost() {
  if (port) return port;
  
  console.log("Connecting to native host 'passmgr'...");
  port = browser.runtime.connectNative("passmgr");
  
  // Écoute les messages retournés par le binaire C
  port.onMessage.addListener((response) => {
    console.log("Received response from native host:", response);
    
    // Résout la plus ancienne promesse en attente
    if (nativeRequestQueue.length > 0) {
      const { resolve } = nativeRequestQueue.shift();
      resolve(response);
    }
    
    // Relaye également la réponse à la Popup (si ouverte) pour compatibilité
    browser.runtime.sendMessage({ target: "popup", data: response }).catch(err => {
      // Ignorer si la popup est fermée
    });
  });
  
  // Gère la déconnexion inattendue ou fermeture du binaire
  port.onDisconnect.addListener((p) => {
    console.log("Disconnected from native host. Error:", p.error);
    port = null;
    
    // Rejeter toutes les promesses en attente
    while (nativeRequestQueue.length > 0) {
      const { reject } = nativeRequestQueue.shift();
      reject(new Error(p.error?.message || "Disconnected from native host"));
    }
    
    // Notifie la popup de la perte de connexion
    browser.runtime.sendMessage({ 
      target: "popup", 
      data: { status: "disconnected", error: p.error?.message } 
    }).catch(err => {});
  });
  
  return port;
}

// Envoie un message au binaire natif de manière synchrone via une Promise
function sendNativeMessage(payload) {
  return new Promise((resolve, reject) => {
    const activePort = connectToNativeHost();
    if (!activePort) {
      return reject(new Error("Failed to connect to native host"));
    }
    
    nativeRequestQueue.push({ resolve, reject });
    try {
      console.log("Sending payload to native host:", payload);
      activePort.postMessage(payload);
    } catch (err) {
      nativeRequestQueue.pop();
      reject(err);
    }
  });
}

// Algorithme de correspondance de domaine
function findMatchingSecret(secrets, domain) {
  const cleanDomain = domain.toLowerCase().trim();
  // 1. Essayer une correspondance exacte
  for (const s of secrets) {
    const title = s.title.toLowerCase().trim();
    if (title === cleanDomain) {
      return s;
    }
  }
  // 2. Essayer une correspondance partielle (ex. le titre est "github" et le domaine est "github.com")
  for (const s of secrets) {
    const title = s.title.toLowerCase().trim();
    if (cleanDomain.includes(title) || title.includes(cleanDomain)) {
      return s;
    }
  }
  return null;
}

// Écoute les requêtes en provenance de la Popup ou du Content Script
browser.runtime.onMessage.addListener((message, sender, sendResponse) => {
  if (message.target === "background") {
    const action = message.action;
    
    if (action === "sendToHost") {
      sendNativeMessage(message.payload)
        .then(response => {
          sendResponse(response);
          // Également envoyé via sendMessage pour la compatibilité avec la popup
          browser.runtime.sendMessage({ target: "popup", data: response }).catch(() => {});
        })
        .catch(err => {
          const errResp = { status: "error", message: err.message };
          sendResponse(errResp);
          browser.runtime.sendMessage({ target: "popup", data: errResp }).catch(() => {});
        });
      return true; // Garde le canal ouvert pour sendResponse
    }
    
    // --- Actions pour le Remplissage/Enregistrement automatique ---
    
    if (action === "check-credentials") {
      const { domain, username, password } = message;
      
      (async () => {
        try {
          // 1. Vérifie si le coffre est déverrouillé
          const status = await sendNativeMessage({ action: "status" });
          if (status.status !== "unlocked") {
            sendResponse({ status: "locked" });
            return;
          }
          
          // 2. Récupère la liste des secrets
          const listResp = await sendNativeMessage({ action: "list" });
          if (listResp.status !== "success" || !listResp.secrets) {
            sendResponse({ status: "error", message: "Failed to list secrets" });
            return;
          }
          
          // 3. Cherche un secret correspondant au domaine
          const match = findMatchingSecret(listResp.secrets, domain);
          
          if (match) {
            // Récupère les détails pour vérifier si les informations de connexion ont changé
            const detail = await sendNativeMessage({ action: "get", title: match.title });
            if (detail.status === "success" && detail.fields) {
              const usernameField = detail.fields.find(f => f.name.toLowerCase() === "login" || f.name.toLowerCase() === "username");
              const passwordField = detail.fields.find(f => f.name.toLowerCase() === "password");
              
              const existingUser = usernameField ? usernameField.value : "";
              const existingPass = passwordField ? passwordField.value : "";
              
              if (existingUser !== username || existingPass !== password) {
                // Suggère d'enregistrer/mettre à jour car les credentials diffèrent
                sendResponse({ 
                  status: "suggest-save", 
                  title: match.title, 
                  exists: true,
                  username: username,
                  password: password
                });
                return;
              }
            }
          } else {
            // Aucun match trouvé, propose d'ajouter ce nouveau secret
            sendResponse({ 
              status: "suggest-save", 
              title: domain, 
              exists: false,
              username: username,
              password: password
            });
            return;
          }
          
          sendResponse({ status: "no-change" });
        } catch (err) {
          console.error("Error checking credentials:", err);
          sendResponse({ status: "error", message: err.message });
        }
      })();
      return true;
    }
    
    if (action === "save-credentials") {
      const { title, type, fields } = message;
      sendNativeMessage({ action: "add", title, type, fields })
        .then(response => {
          sendResponse(response);
        })
        .catch(err => {
          sendResponse({ status: "error", message: err.message });
        });
      return true;
    }
  }
  return true;
});

// Première connexion au lancement
connectToNativeHost();