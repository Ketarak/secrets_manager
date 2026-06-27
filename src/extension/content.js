(function() {
  // Éviter la double injection
  if (window.__passmgrContentScriptLoaded) return;
  window.__passmgrContentScriptLoaded = true;

  console.log("Secrets Manager content script active.");

  // Variables de session temporaires pour stocker les données du formulaire avant soumission
  let lastCapturedUsername = "";
  let lastCapturedPassword = "";
  let captureTimeout = null;

  // Variables pour le portail de suggestions
  let activeInputPair = null; // Paire d'inputs courante à remplir
  let portalHost = null; // Conteneur du Shadow DOM pour le dropdown global

  // Icône clé SVG
  const keyIconSvg = `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" style="width: 14px; height: 14px; display: block;"><path d="M21 2l-2 2m-7.61 7.61a5.5 5.5 0 1 1-7.778 7.778 5.5 5.5 0 0 1 7.777-7.777zm0 0L15.5 7.5m0 0l3 3L22 7l-3-3-3.5 3.5z"></path></svg>`;

  // Sécurise l'effacement des variables sensibles
  function wipeSensitiveData() {
    lastCapturedUsername = "";
    lastCapturedPassword = "";
  }

  // --- 1. Remplissage des champs (Injection robuste) ---

  function fillField(inputEl, value) {
    if (!inputEl) return;
    try {
      // Utilisation du prototype natif de HTMLInputElement pour outrepasser les frameworks (React/Vue/Angular)
      const setter = Object.getOwnPropertyDescriptor(HTMLInputElement.prototype, 'value').set;
      if (setter) {
        setter.call(inputEl, value);
      } else {
        inputEl.value = value;
      }
      // Déclenche les événements d'input pour mettre à jour les états des frameworks
      inputEl.dispatchEvent(new Event('input', { bubbles: true }));
      inputEl.dispatchEvent(new Event('change', { bubbles: true }));
    } catch (e) {
      inputEl.value = value;
      inputEl.dispatchEvent(new Event('input', { bubbles: true }));
      inputEl.dispatchEvent(new Event('change', { bubbles: true }));
    }
  }

  // Effectue le remplissage d'un lot de champs sur une paire détectée
  function performAutofill(pair, fields) {
    const usernameField = fields.find(f => f.name.toLowerCase() === "login" || f.name.toLowerCase() === "username" || f.name.toLowerCase() === "identifiant");
    const passwordField = fields.find(f => f.name.toLowerCase() === "password" || f.name.toLowerCase() === "mot de passe");

    if (usernameField && pair.username) {
      fillField(pair.username, usernameField.value);
    }
    if (passwordField && pair.password) {
      fillField(pair.password, passwordField.value);
    }
  }

  // Écoute les messages en provenance du background script (Raccourci clavier ou Bouton popup)
  browser.runtime.onMessage.addListener((message, sender, sendResponse) => {
    if (message.target === "content") {
      const action = message.action;

      if (action === "fill-credentials") {
        const pairs = findLoginFields();
        if (pairs.length > 0) {
          // Remplir la première paire trouvée (ou celle actuellement active si possible)
          const activeElement = document.activeElement;
          let targetPair = pairs[0];
          
          // Si le focus est sur l'un des inputs d'une paire, on cible cette paire
          const focusedPair = pairs.find(p => p.username === activeElement || p.password === activeElement);
          if (focusedPair) {
            targetPair = focusedPair;
          }
          
          performAutofill(targetPair, message.fields);
          console.log("[Secrets Manager] Fields successfully autofilled.");
        }
      }
    }
  });

  // --- 2. Détection des champs & Décoration ---

  // Détecte les paires de champs identifiant/mot de passe sur la page
  function findLoginFields() {
    const passwordInputs = Array.from(document.querySelectorAll('input[type="password"]'));
    const pairs = [];

    passwordInputs.forEach(passInput => {
      // Ignorer les champs masqués ou invisibles
      if (passInput.offsetWidth === 0 || passInput.offsetHeight === 0) return;

      const form = passInput.form;
      let userInput = null;

      if (form) {
        const inputs = Array.from(form.querySelectorAll('input'));
        const passIndex = inputs.indexOf(passInput);
        for (let i = passIndex - 1; i >= 0; i--) {
          const type = inputs[i].type.toLowerCase();
          if (type === "text" || type === "email" || type === "username") {
            userInput = inputs[i];
            break;
          }
        }
      }

      if (!userInput) {
        // Recherche globale dans le DOM avant le mot de passe
        const allInputs = Array.from(document.querySelectorAll('input'));
        const passIndex = allInputs.indexOf(passInput);
        for (let i = passIndex - 1; i >= 0; i--) {
          const type = allInputs[i].type.toLowerCase();
          if (type === "text" || type === "email" || type === "username") {
            userInput = allInputs[i];
            break;
          }
        }
      }

      if (userInput) {
        pairs.push({ username: userInput, password: passInput });
      }
    });

    return pairs;
  }

  // Injecte un bouton clé cliquable à la fin d'un champ input
  function decorateInput(inputEl, pair) {
    if (inputEl.dataset.passmgrDecorated) return;
    inputEl.dataset.passmgrDecorated = "true";

    const parent = inputEl.parentElement;
    if (!parent) return;

    // Assurer que le parent est positionné pour placer le bouton clé
    const parentStyle = window.getComputedStyle(parent);
    if (parentStyle.position === "static") {
      parent.style.position = "relative";
    }

    // Crée le conteneur du bouton clé dans la page (Shadow DOM)
    const host = document.createElement("div");
    host.className = "passmgr-key-host";
    host.style.position = "absolute";
    host.style.right = "10px";
    host.style.top = "50%";
    host.style.transform = "translateY(-50%)";
    host.style.zIndex = "1000";
    host.style.width = "20px";
    host.style.height = "20px";
    host.style.cursor = "pointer";

    // Ajuste le padding droit de l'input pour laisser la place à l'icône clé
    const currentPaddingRight = parseFloat(window.getComputedStyle(inputEl).paddingRight) || 0;
    if (currentPaddingRight < 30) {
      inputEl.style.paddingRight = `${currentPaddingRight + 20}px`;
    }

    const shadow = host.attachShadow({ mode: "closed" });

    // CSS du bouton clé
    const style = document.createElement("style");
    style.textContent = `
      .key-btn {
        background: none;
        border: none;
        color: #9ca3af;
        cursor: pointer;
        padding: 3px;
        border-radius: 4px;
        transition: all 0.2s;
        display: flex;
        align-items: center;
        justify-content: center;
      }
      .key-btn:hover {
        color: #6366f1;
        background: rgba(99, 102, 241, 0.1);
        transform: scale(1.05);
      }
    `;
    shadow.appendChild(style);

    const btn = document.createElement("button");
    btn.type = "button";
    btn.className = "key-btn";
    btn.innerHTML = keyIconSvg;
    
    btn.addEventListener("click", (e) => {
      e.stopPropagation();
      e.preventDefault();
      activeInputPair = pair;
      togglePortalSuggestions(inputEl);
    });

    shadow.appendChild(btn);
    parent.appendChild(host);
  }

  // --- 3. Portail Suggestions In-Page (Shadow DOM) ---

  // Crée et affiche le dropdown de suggestions près de l'input cliqué
  function togglePortalSuggestions(targetInput) {
    // Si le portail est déjà ouvert pour cet input, on le ferme
    if (portalHost && portalHost.dataset.targetInput === targetInput.id) {
      closePortalSuggestions();
      return;
    }

    closePortalSuggestions(); // Ferme tout portail existant

    // Demande les suggestions correspondantes pour ce site
    browser.runtime.sendMessage({
      target: "background",
      action: "get-active-tab-suggestions"
    }).then(response => {
      if (response && response.status === "success" && response.suggestions && response.suggestions.length > 0) {
        showPortalDropdown(targetInput, response.suggestions);
      } else if (response && response.status === "locked") {
        showPortalDropdown(targetInput, [{ labelOnly: true, text: "Coffre verrouillé. Ouvrez l'extension pour déverrouiller." }]);
      } else {
        showPortalDropdown(targetInput, [{ labelOnly: true, text: "Aucun identifiant trouvé." }]);
      }
    }).catch(err => {
      console.error("[Secrets Manager] Error fetching in-page suggestions:", err);
    });
  }

  function showPortalDropdown(targetInput, items) {
    portalHost = document.createElement("div");
    portalHost.id = "passmgr-portal-host";
    portalHost.dataset.targetInput = targetInput.id;
    
    // Positionnement absolu sur le document pour éviter le clipping par overflow:hidden
    const rect = targetInput.getBoundingClientRect();
    const scrollTop = window.pageYOffset || document.documentElement.scrollTop;
    const scrollLeft = window.pageXOffset || document.documentElement.scrollLeft;

    portalHost.style.position = "absolute";
    portalHost.style.top = `${rect.bottom + scrollTop + 4}px`;
    portalHost.style.left = `${rect.left + scrollLeft}px`;
    portalHost.style.width = `${Math.max(rect.width, 240)}px`;
    portalHost.style.zIndex = "2147483647"; // Au-dessus de tout

    const shadow = portalHost.attachShadow({ mode: "closed" });

    // Styles du Dropdown
    const style = document.createElement("style");
    style.textContent = `
      .dropdown-container {
        background: rgba(15, 12, 27, 0.95);
        backdrop-filter: blur(12px);
        -webkit-backdrop-filter: blur(12px);
        border: 1px solid rgba(255, 255, 255, 0.08);
        border-radius: 8px;
        box-shadow: 0 10px 25px rgba(0, 0, 0, 0.4), 0 0 1px rgba(99, 102, 241, 0.2);
        padding: 6px;
        box-sizing: border-box;
        font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
        color: #f3f4f6;
        animation: fadeIn 0.2s ease-out;
      }
      @keyframes fadeIn {
        from { opacity: 0; transform: translateY(-4px); }
        to { opacity: 1; transform: translateY(0); }
      }
      .dropdown-item {
        display: flex;
        flex-direction: column;
        padding: 8px 12px;
        border-radius: 6px;
        cursor: pointer;
        transition: background 0.15s;
      }
      .dropdown-item:hover {
        background: rgba(99, 102, 241, 0.15);
      }
      .item-title {
        font-size: 12px;
        font-weight: 600;
        color: #f3f4f6;
      }
      .item-user {
        font-size: 11px;
        color: #9ca3af;
        font-family: monospace;
        margin-top: 2px;
      }
      .info-label {
        padding: 8px 12px;
        font-size: 11px;
        color: #9ca3af;
        text-align: center;
      }
    `;
    shadow.appendChild(style);

    const container = document.createElement("div");
    container.className = "dropdown-container";

    items.forEach(item => {
      const el = document.createElement("div");
      if (item.labelOnly) {
        el.className = "info-label";
        el.innerText = item.text;
      } else {
        el.className = "dropdown-item";
        el.innerHTML = `
          <span class="item-title">${escapeHtml(item.title)}</span>
          <span class="item-user">${escapeHtml(item.username)}</span>
        `;
        el.addEventListener("click", () => {
          if (activeInputPair) {
            performAutofill(activeInputPair, item.fields);
            closePortalSuggestions();
          }
        });
      }
      container.appendChild(el);
    });

    shadow.appendChild(container);
    document.body.appendChild(portalHost);

    // Écouteur de clic externe pour fermer le dropdown
    setTimeout(() => {
      document.addEventListener("click", handleExternalClick);
    }, 10);
  }

  function closePortalSuggestions() {
    if (portalHost) {
      portalHost.remove();
      portalHost = null;
    }
    document.removeEventListener("click", handleExternalClick);
  }

  function handleExternalClick(e) {
    if (portalHost) {
      closePortalSuggestions();
    }
  }

  // --- 4. Capture automatique des credentials lors de submit ---

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
    }, 30000);
  }, true);

  // Affiche la bannière d'invite d'enregistrement sécurisée (Shadow DOM)
  function showSavePromptBanner(title, exists) {
    const existingHost = document.getElementById("passmgr-banner-host");
    if (existingHost) existingHost.remove();

    const hostDiv = document.createElement("div");
    hostDiv.id = "passmgr-banner-host";
    
    hostDiv.style.position = "fixed";
    hostDiv.style.top = "0";
    hostDiv.style.left = "0";
    hostDiv.style.width = "100vw";
    hostDiv.style.zIndex = "2147483647";
    hostDiv.style.pointerEvents = "none";

    const shadowRoot = hostDiv.attachShadow({ mode: "closed" });

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
        to { transform: translateY(0); opacity: 1; }
      }
      .banner-card.slide-out {
        animation: slideOut 0.3s cubic-bezier(0.7, 0, 0.84, 0) forwards;
      }
      @keyframes slideOut {
        to { transform: translateY(-80px); opacity: 0; }
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

    const wrapper = document.createElement("div");
    wrapper.className = "banner-wrapper";

    const card = document.createElement("div");
    card.className = "banner-card";
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

    let autoDismissTimer = setTimeout(() => {
      dismissBanner();
    }, 15000);

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

    const ignoreBtn = card.querySelector("#btn-ignore");
    ignoreBtn.addEventListener("click", () => {
      dismissBanner();
    });

    const saveBtn = card.querySelector("#btn-save");
    saveBtn.addEventListener("click", () => {
      if (autoDismissTimer) clearTimeout(autoDismissTimer);
      saveBtn.disabled = true;
      ignoreBtn.disabled = true;
      saveBtn.innerText = "Enregistrement...";

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

  // --- 5. Initialisation : Scan et observation ---

  function scanAndDecorate() {
    const pairs = findLoginFields();
    pairs.forEach(pair => {
      if (pair.username) decorateInput(pair.username, pair);
      if (pair.password) decorateInput(pair.password, pair);
    });
  }

  // Lancement initial
  scanAndDecorate();

  // Surveille l'insertion dynamique d'inputs par des frameworks JS
  const observer = new MutationObserver(() => {
    scanAndDecorate();
  });
  observer.observe(document.body, { childList: true, subtree: true });

  function escapeHtml(str) {
    if (!str) return "";
    return str.replace(/&/g, "&amp;")
              .replace(/</g, "&lt;")
              .replace(/>/g, "&gt;")
              .replace(/"/g, "&quot;")
              .replace(/'/g, "&#039;");
  }
})();
