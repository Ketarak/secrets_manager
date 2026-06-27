(function() {
  // Éviter la double injection
  if (window.__passmgrContentScriptLoaded) return;
  window.__passmgrContentScriptLoaded = true;

  console.log("Secrets Manager content script active.");

  // Variables de session temporaires pour stocker les données du formulaire avant soumission
  let lastCapturedUsername = "";
  let lastCapturedPassword = "";
  let captureTimeout = null;

  // Sécurise l'effacement des variables sensibles
  function wipeSensitiveData() {
    lastCapturedUsername = "";
    lastCapturedPassword = "";
  }

  // Intercepte les soumissions de formulaire
  document.addEventListener("submit", (event) => {
    const form = event.target;
    
    // Recherche un champ mot de passe dans le formulaire soumis
    const passwordInput = form.querySelector('input[type="password"]');
    if (!passwordInput) return;

    // Recherche l'identifiant (le premier input de type text/email avant le mot de passe)
    const inputs = Array.from(form.querySelectorAll('input'));
    const passIndex = inputs.indexOf(passwordInput);
    let usernameInput = null;

    for (let i = passIndex - 1; i >= 0; i--) {
      const type = inputs[i].type.toLowerCase();
      if (type === "text" || type === "email" || type === "username") {
        usernameInput = inputs[i];
        break;
      }
    }

    if (!usernameInput || !usernameInput.value || !passwordInput.value) return;

    // Sauvegarde temporaire des valeurs pour la vérification
    lastCapturedUsername = usernameInput.value.trim();
    lastCapturedPassword = passwordInput.value;

    console.log("[Secrets Manager] Form submission captured.");

    // Envoi de la demande de vérification au script background
    browser.runtime.sendMessage({
      target: "background",
      action: "check-credentials",
      domain: window.location.hostname,
      username: lastCapturedUsername,
      password: lastCapturedPassword
    }).then((response) => {
      if (response && response.status === "suggest-save") {
        showSavePromptBanner(response.title, response.exists);
      } else {
        // Pas de changement ou coffre verrouillé, on efface immédiatement les données sensibles
        wipeSensitiveData();
      }
    }).catch(err => {
      console.error("[Secrets Manager] Error checking credentials:", err);
      wipeSensitiveData();
    });

    // Planifie un effacement de sécurité au cas où l'utilisateur n'interagit pas
    if (captureTimeout) clearTimeout(captureTimeout);
    captureTimeout = setTimeout(() => {
      wipeSensitiveData();
    }, 30000); // 30 secondes de tolérance max
  }, true);

  // Affiche la bannière d'invite d'enregistrement sécurisée
  function showSavePromptBanner(title, exists) {
    // Supprime l'ancienne bannière si elle existe déjà
    const existingHost = document.getElementById("passmgr-banner-host");
    if (existingHost) existingHost.remove();

    // Crée un conteneur hôte pour le Shadow DOM
    const hostDiv = document.createElement("div");
    hostDiv.id = "passmgr-banner-host";
    
    // Force une isolation CSS
    hostDiv.style.position = "fixed";
    hostDiv.style.top = "0";
    hostDiv.style.left = "0";
    hostDiv.style.width = "100vw";
    hostDiv.style.zIndex = "2147483647"; // Au-dessus de tous les éléments du site
    hostDiv.style.pointerEvents = "none";

    const shadowRoot = hostDiv.attachShadow({ mode: "closed" });

    // CSS injecté directement dans le Shadow DOM pour garantir son apparence
    const cssStyle = document.createElement("style");
    cssStyle.textContent = `
      .banner-wrapper {
        position: relative;
        display: flex;
        justify-content: center;
        width: 100%;
        padding: 12px;
        box-sizing: border-box;
      }
      .banner-card {
        pointer-events: auto;
        display: flex;
        align-items: center;
        justify-content: space-between;
        gap: 20px;
        min-width: 480px;
        max-width: 700px;
        background: rgba(15, 12, 27, 0.85);
        backdrop-filter: blur(14px);
        -webkit-backdrop-filter: blur(14px);
        border: 1px solid rgba(255, 255, 255, 0.08);
        box-shadow: 0 10px 30px rgba(0, 0, 0, 0.5), 0 0 1px rgba(99, 102, 241, 0.2);
        padding: 12px 20px;
        border-radius: 12px;
        font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
        color: #f3f4f6;
        animation: slideIn 0.35s cubic-bezier(0.16, 1, 0.3, 1) forwards;
        transform: translateY(-80px);
        opacity: 0;
      }
      @keyframes slideIn {
        to {
          transform: translateY(0);
          opacity: 1;
        }
      }
      .banner-card.slide-out {
        animation: slideOut 0.3s cubic-bezier(0.7, 0, 0.84, 0) forwards;
      }
      @keyframes slideOut {
        to {
          transform: translateY(-80px);
          opacity: 0;
        }
      }
      .banner-content {
        display: flex;
        align-items: center;
        gap: 12px;
      }
      .banner-logo {
        width: 22px;
        height: 22px;
        color: #6366f1;
        flex-shrink: 0;
      }
      .banner-text-group {
        display: flex;
        flex-direction: column;
        gap: 2px;
      }
      .banner-title {
        font-size: 13px;
        font-weight: 600;
        letter-spacing: 0.2px;
      }
      .banner-subtext {
        font-size: 11px;
        color: #9ca3af;
      }
      .banner-actions {
        display: flex;
        align-items: center;
        gap: 8px;
        flex-shrink: 0;
      }
      .btn {
        padding: 7px 14px;
        border-radius: 6px;
        font-size: 12px;
        font-weight: 500;
        cursor: pointer;
        border: none;
        transition: all 0.2s ease;
      }
      .btn-primary {
        background: linear-gradient(135deg, #6366f1 0%, #a855f7 100%);
        color: #ffffff;
        box-shadow: 0 2px 6px rgba(99, 102, 241, 0.25);
      }
      .btn-primary:hover {
        opacity: 0.95;
        transform: translateY(-0.5px);
        box-shadow: 0 3px 8px rgba(99, 102, 241, 0.4);
      }
      .btn-secondary {
        background: rgba(255, 255, 255, 0.05);
        color: #e5e7eb;
        border: 1px solid rgba(255, 255, 255, 0.08);
      }
      .btn-secondary:hover {
        background: rgba(255, 255, 255, 0.1);
        color: #ffffff;
      }
      .success-title {
        color: #10b981 !important;
      }
    `;
    shadowRoot.appendChild(cssStyle);

    // Template HTML de la bannière
    const wrapper = document.createElement("div");
    wrapper.className = "banner-wrapper";

    const card = document.createElement("div");
    card.className = "banner-card";
    
    // Logo Cadenas SVG
    const logoSvg = `<svg class="banner-logo" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"></rect><path d="M7 11V7a5 5 0 0 1 10 0v4"></path></svg>`;

    card.innerHTML = `
      <div class="banner-content">
        ${logoSvg}
        <div class="banner-text-group">
          <span class="banner-title">${exists ? "Mettre à jour le mot de passe ?" : "Enregistrer l'identifiant ?"}</span>
          <span class="banner-subtext">Compte : ${escapeHtml(lastCapturedUsername)} pour ${escapeHtml(title)}</span>
        </div>
      </div>
      <div class="banner-actions">
        <button id="btn-ignore" class="btn btn-secondary">Ignorer</button>
        <button id="btn-save" class="btn btn-primary">${exists ? "Mettre à jour" : "Enregistrer"}</button>
      </div>
    `;
    
    wrapper.appendChild(card);
    shadowRoot.appendChild(wrapper);
    document.body.appendChild(hostDiv);

    // Durée de vie de 15 secondes avant auto-slide-out
    let autoDismissTimer = setTimeout(() => {
      dismissBanner();
    }, 15000);

    // Fonction de suppression de la bannière avec animation
    function dismissBanner() {
      if (autoDismissTimer) clearTimeout(autoDismissTimer);
      card.classList.add("slide-out");
      card.addEventListener("animationend", (e) => {
        if (e.animationName === "slideOut") {
          hostDiv.remove();
          wipeSensitiveData();
        }
      });
    }

    // Gestionnaires de clic des boutons de la bannière
    const ignoreBtn = card.querySelector("#btn-ignore");
    ignoreBtn.addEventListener("click", () => {
      dismissBanner();
    });

    const saveBtn = card.querySelector("#btn-save");
    saveBtn.addEventListener("click", () => {
      if (autoDismissTimer) clearTimeout(autoDismissTimer);
      
      // Désactiver les boutons pendant l'enregistrement
      saveBtn.disabled = true;
      ignoreBtn.disabled = true;
      saveBtn.innerText = "Enregistrement...";

      // Envoie le message d'enregistrement sécurisé au background
      browser.runtime.sendMessage({
        target: "background",
        action: "save-credentials",
        title: title,
        type: "login",
        fields: [
          { name: "login", value: lastCapturedUsername, is_sensitive: false },
          { name: "password", value: lastCapturedPassword, is_sensitive: true }
        ]
      }).then((resp) => {
        if (resp && resp.status === "success") {
          // Affichage de réussite
          const textGroup = card.querySelector(".banner-text-group");
          textGroup.innerHTML = `
            <span class="banner-title success-title">Enregistré dans Secrets Manager ! 🎉</span>
            <span class="banner-subtext">Coffre-fort mis à jour avec succès.</span>
          `;
          card.querySelector(".banner-actions").style.display = "none";
          
          setTimeout(() => {
            dismissBanner();
          }, 2000);
        } else {
          // Affichage d'erreur
          const titleSpan = card.querySelector(".banner-title");
          titleSpan.innerText = "Erreur de sauvegarde";
          titleSpan.style.color = "#fca5a5";
          saveBtn.innerText = "Réessayer";
          saveBtn.disabled = false;
          ignoreBtn.disabled = false;
        }
      }).catch(err => {
        console.error("[Secrets Manager] Failed to save credentials:", err);
        dismissBanner();
      });
    });
  }

  // Utilitaire d'échappement HTML
  function escapeHtml(str) {
    if (!str) return "";
    return str.replace(/&/g, "&amp;")
              .replace(/</g, "&lt;")
              .replace(/>/g, "&gt;")
              .replace(/"/g, "&quot;")
              .replace(/'/g, "&#039;");
  }
})();
