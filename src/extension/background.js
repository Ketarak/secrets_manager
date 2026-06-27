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

// Récupère l'onglet actif courant
async function getActiveTab() {
  const tabs = await browser.tabs.query({ active: true, currentWindow: true });
  return tabs[0];
}

// Extrait le domaine propre à partir d'une URL
function getDomainFromUrl(url) {
  try {
    const parsed = new URL(url);
    let host = parsed.hostname;
    if (host.startsWith("www.")) {
      host = host.substring(4);
    }
    return host;
  } catch (e) {
    return "";
  }
}

// Algorithme de correspondance de domaine unique
function findMatchingSecret(secrets, domain) {
  const cleanDomain = domain.toLowerCase().trim();
  // 1. Essayer une correspondance exacte
  for (const s of secrets) {
    const title = s.title.toLowerCase().trim();
    if (title === cleanDomain) {
      return s;
    }
  }
  // 2. Essayer une correspondance partielle
  for (const s of secrets) {
    const title = s.title.toLowerCase().trim();
    if (cleanDomain.includes(title) || title.includes(cleanDomain)) {
      return s;
    }
  }
  return null;
}

// Algorithme de correspondance de domaine retournant TOUS les secrets correspondants
function findMatchingSecrets(secrets, domain) {
  const cleanDomain = domain.toLowerCase().trim();
  const matches = [];
  
  // 1. Correspondance exacte
  for (const s of secrets) {
    const title = s.title.toLowerCase().trim();
    if (title === cleanDomain) {
      matches.push(s);
    }
  }
  // 2. Correspondance partielle (si non déjà ajouté et contient/est contenu)
  for (const s of secrets) {
    const title = s.title.toLowerCase().trim();
    if (title !== cleanDomain && (cleanDomain.includes(title) || title.includes(cleanDomain))) {
      matches.push(s);
    }
  }
  return matches;
}

// Écouteur de raccourci clavier global (Ctrl+Shift+L)
browser.commands.onCommand.addListener(async (command) => {
  if (command === "trigger-autofill") {
    console.log("[Secrets Manager] Keyboard shortcut trigger-autofill detected.");
    try {
      const tab = await getActiveTab();
      if (!tab || !tab.url) return;
      
      const domain = getDomainFromUrl(tab.url);
      if (!domain) return;
      
      // 1. Vérifie si le coffre est déverrouillé
      const status = await sendNativeMessage({ action: "status" });
      if (status.status !== "unlocked") {
        console.log("[Secrets Manager] Autofill ignored: Vault is locked.");
        return;
      }
      
      // 2. Récupère la liste des secrets
      const listResp = await sendNativeMessage({ action: "list" });
      if (listResp.status !== "success" || !listResp.secrets) return;
      
      // 3. Cherche un secret correspondant au domaine
      const match = findMatchingSecret(listResp.secrets, domain);
      if (!match) {
        console.log("[Secrets Manager] No matching secret found for domain:", domain);
        return;
      }
      
      // 4. Récupère les détails du secret
      const detail = await sendNativeMessage({ action: "get", title: match.title });
      if (detail.status === "success" && detail.fields) {
        // Envoie les identifiants au script de contenu de l'onglet actif
        browser.tabs.sendMessage(tab.id, {
          target: "content",
          action: "fill-credentials",
          fields: detail.fields
        }).catch(err => {
          console.error("[Secrets Manager] Failed to send fill message to tab:", err);
        });
      }
    } catch (err) {
      console.error("[Secrets Manager] Error handling trigger-autofill shortcut:", err);
    }
  }
});

// Écoute les requêtes en provenance de la Popup ou du Content Script
browser.runtime.onMessage.addListener((message, sender, sendResponse) => {
  if (message.target === "background") {
    const action = message.action;
    
    if (action === "sendToHost") {
      sendNativeMessage(message.payload)
        .then(response => {
          sendResponse(response);
          browser.runtime.sendMessage({ target: "popup", data: response }).catch(() => {});
        })
        .catch(err => {
          const errResp = { status: "error", message: err.message };
          sendResponse(errResp);
          browser.runtime.sendMessage({ target: "popup", data: errResp }).catch(() => {});
        });
      return true;
    }
    
    // --- Actions pour le Remplissage/Enregistrement automatique ---
    
    if (action === "check-credentials") {
      const { domain, username, password } = message;
      
      (async () => {
        try {
          const status = await sendNativeMessage({ action: "status" });
          if (status.status !== "unlocked") {
            sendResponse({ status: "locked" });
            return;
          }
          
          const listResp = await sendNativeMessage({ action: "list" });
          if (listResp.status !== "success" || !listResp.secrets) {
            sendResponse({ status: "error", message: "Failed to list secrets" });
            return;
          }
          
          const match = findMatchingSecret(listResp.secrets, domain);
          
          if (match) {
            const detail = await sendNativeMessage({ action: "get", title: match.title });
            if (detail.status === "success" && detail.fields) {
              const usernameField = detail.fields.find(f => f.name.toLowerCase() === "login" || f.name.toLowerCase() === "username");
              const passwordField = detail.fields.find(f => f.name.toLowerCase() === "password");
              
              const existingUser = usernameField ? usernameField.value : "";
              const existingPass = passwordField ? passwordField.value : "";
              
              if (existingUser !== username || existingPass !== password) {
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

    // Récupère les suggestions de remplissage correspondant à l'onglet actif courant
    if (action === "get-active-tab-suggestions") {
      (async () => {
        try {
          const tab = await getActiveTab();
          if (!tab || !tab.url) {
            sendResponse({ status: "success", suggestions: [] });
            return;
          }

          const domain = getDomainFromUrl(tab.url);
          if (!domain) {
            sendResponse({ status: "success", suggestions: [] });
            return;
          }

          const status = await sendNativeMessage({ action: "status" });
          if (status.status !== "unlocked") {
            sendResponse({ status: "locked" });
            return;
          }

          const listResp = await sendNativeMessage({ action: "list" });
          if (listResp.status !== "success" || !listResp.secrets) {
            sendResponse({ status: "error", message: "Failed to list secrets" });
            return;
          }

          // Trouve tous les secrets qui correspondent au domaine
          const matches = findMatchingSecrets(listResp.secrets, domain);
          const suggestedSecrets = [];

          for (const m of matches) {
            const detail = await sendNativeMessage({ action: "get", title: m.title });
            if (detail.status === "success" && detail.fields) {
              const usernameField = detail.fields.find(f => f.name.toLowerCase() === "login" || f.name.toLowerCase() === "username" || f.name.toLowerCase() === "identifiant");
              suggestedSecrets.push({
                title: m.title,
                type: m.type,
                username: usernameField ? usernameField.value : "",
                fields: detail.fields
              });
            }
          }

          sendResponse({ status: "success", suggestions: suggestedSecrets, domain: domain });
        } catch (err) {
          console.error("Error getting active tab suggestions:", err);
          sendResponse({ status: "error", message: err.message });
        }
      })();
      return true;
    }

    // Effectue le remplissage des credentials sur la page de l'onglet actif
    if (action === "fill-credentials-on-page") {
      const { fields } = message;
      (async () => {
        try {
          const tab = await getActiveTab();
          if (tab) {
            browser.tabs.sendMessage(tab.id, {
              target: "content",
              action: "fill-credentials",
              fields: fields
            }).catch(err => {
              console.error("[Secrets Manager] Failed to send fill message to content script:", err);
            });
            sendResponse({ status: "success" });
          } else {
            sendResponse({ status: "error", message: "No active tab found" });
          }
        } catch (err) {
          sendResponse({ status: "error", message: err.message });
        }
      })();
      return true;
    }
  }
  return true;
});

// Première connexion au lancement
connectToNativeHost();