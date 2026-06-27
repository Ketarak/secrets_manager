document.addEventListener("DOMContentLoaded", () => {
  // --- Éléments de l'interface ---
  const lockedView = document.getElementById("locked-view");
  const createView = document.getElementById("create-view");
  const unlockedView = document.getElementById("unlocked-view");
  const statusDot = document.getElementById("status-dot");
  const statusText = document.getElementById("status-text");

  // Locked View Elements
  const unlockForm = document.getElementById("unlock-form");
  const masterPasswordInput = document.getElementById("master-password");
  const toggleMasterPwdBtn = document.getElementById("toggle-master-pwd");
  const unlockBtn = document.getElementById("unlock-btn");
  const unlockError = document.getElementById("unlock-error");

  // Create View Elements
  const createForm = document.getElementById("create-form");
  const createPasswordInput = document.getElementById("create-password");
  const createConfirmPasswordInput = document.getElementById("create-confirm-password");
  const toggleCreatePwdBtn = document.getElementById("toggle-create-pwd");
  const toggleCreateConfirmPwdBtn = document.getElementById("toggle-create-confirm-pwd");
  const createBtn = document.getElementById("create-btn");
  const createError = document.getElementById("create-error");

  // Unlocked View Elements
  const searchInput = document.getElementById("search-input");
  const secretsList = document.getElementById("secrets-list");
  const noSecretsMsg = document.getElementById("no-secrets-msg");
  const lockBtn = document.getElementById("lock-btn");

  // Detail Subview Elements
  const searchSubview = document.getElementById("search-subview");
  const detailSubview = document.getElementById("detail-subview");
  const backToListBtn = document.getElementById("back-to-list-btn");
  const detailTitle = document.getElementById("detail-title");
  const detailType = document.getElementById("detail-type");
  const detailSecretIcon = document.getElementById("detail-secret-icon");
  const detailFieldsList = document.getElementById("detail-fields-list");

  // --- Session Data ---
  let allSecrets = []; // Tous les secrets chargés depuis le binaire

  // Icônes SVG partagées
  const icons = {
    login: `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"></path><circle cx="12" cy="7" r="4"></circle></svg>`,
    ssh: `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="2" y="4" width="20" height="16" rx="2" ry="2"></rect><line x1="6" y1="8" x2="10" y2="8"></line><line x1="6" y1="12" x2="18" y2="12"></line><line x1="6" y1="16" x2="14" y2="16"></line></svg>`,
    card: `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="2" y="5" width="20" height="14" rx="2" ry="2"></rect><line x1="2" y1="10" x2="22" y2="10"></line></svg>`,
    note: `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"></path><polyline points="14 2 14 8 20 8"></polyline><line x1="16" y1="13" x2="8" y2="13"></line><line x1="16" y1="17" x2="8" y2="17"></line><polyline points="10 9 9 9 8 9"></polyline></svg>`,
    copy: `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="9" y="9" width="13" height="13" rx="2" ry="2"></rect><path d="M5 15H4a2 2 0 0 1-2-2V4a2 2 0 0 1 2-2h9a2 2 0 0 1 2 2v1"></path></svg>`,
    check: `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="feather feather-check"><polyline points="20 6 9 17 4 12"></polyline></svg>`,
    eye: `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path><circle cx="12" cy="12" r="3"></circle></svg>`,
    eyeOff: `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M17.94 17.94A10.07 10.07 0 0 1 12 20c-7 0-11-8-11-8a18.45 18.45 0 0 1 5.06-5.94M9.9 4.24A9.12 9.12 0 0 1 12 4c7 0 11 8 11 8a18.5 18.5 0 0 1-2.16 3.19m-6.72-1.07a3 3 0 1 1-4.24-4.24"></path><line x1="1" y1="1" x2="23" y2="23"></line></svg>`
  };

  // --- Fonctions de Communication ---
  function sendToHost(payload) {
    browser.runtime.sendMessage({
      target: "background",
      action: "sendToHost",
      payload: payload
    });
  }

  // Écoute les retours envoyés par le script de background
  browser.runtime.onMessage.addListener((message) => {
    if (message.target === "popup") {
      const data = message.data;
      console.log("Popup received message:", data);

      if (data.status === "unlocked") {
        updateUIState("unlocked");
        sendToHost({ action: "list" }); // Demande la liste des secrets
      } else if (data.status === "locked" || data.status === "disconnected") {
        if (data.status === "locked" && data.vault_exists === false) {
          updateUIState("no_vault");
        } else {
          updateUIState("locked");
          if (data.error) {
            showError("Connexion au coffre perdue.");
          }
        }
      } else if (data.status === "success") {
        if (data.message === "Vault unlocked" || data.message === "Vault created and unlocked") {
          updateUIState("unlocked");
          sendToHost({ action: "list" });
        } else if (data.message === "Vault locked") {
          updateUIState("locked");
        } else if (data.secrets) {
          allSecrets = data.secrets;
          renderSecretsList(allSecrets);
        } else if (data.fields) {
          showSecretDetails(data);
        }
      } else if (data.status === "error") {
        unlockBtn.disabled = false;
        unlockBtn.innerText = "Déverrouiller";
        createBtn.disabled = false;
        createBtn.innerText = "Créer le coffre";
        if (createView && createView.classList.contains("active")) {
          showCreateError(data.message || "Une erreur est survenue.");
        } else {
          showError(data.message || "Une erreur est survenue.");
        }
      }
    }
  });

  // --- Gestion de l'Interface UI ---
  function updateUIState(state) {
    lockedView.classList.add("hidden");
    lockedView.classList.remove("active");
    if (createView) {
      createView.classList.add("hidden");
      createView.classList.remove("active");
    }
    unlockedView.classList.add("hidden");
    unlockedView.classList.remove("active");

    if (state === "unlocked") {
      unlockedView.classList.remove("hidden");
      unlockedView.classList.add("active");
      statusDot.className = "dot unlocked";
      statusText.innerText = "Déverrouillé";
      unlockError.classList.add("hidden");
      if (createError) createError.classList.add("hidden");
      masterPasswordInput.value = "";
      if (createPasswordInput) createPasswordInput.value = "";
      if (createConfirmPasswordInput) createConfirmPasswordInput.value = "";
    } else if (state === "no_vault") {
      if (createView) {
        createView.classList.remove("hidden");
        createView.classList.add("active");
      }
      statusDot.className = "dot locked";
      statusText.innerText = "Non initialisé";
    } else { // "locked"
      lockedView.classList.remove("hidden");
      lockedView.classList.add("active");
      statusDot.className = "dot locked";
      statusText.innerText = "Verrouillé";
      searchSubview.classList.remove("hidden");
      detailSubview.classList.add("hidden");
    }
  }

  function showError(msg) {
    unlockError.innerText = msg;
    unlockError.classList.remove("hidden");
  }

  function showCreateError(msg) {
    if (createError) {
      createError.innerText = msg;
      createError.classList.remove("hidden");
    }
  }

  // --- Rendu UI ---
  function renderSecretsList(secrets) {
    secretsList.innerHTML = "";
    if (secrets.length === 0) {
      noSecretsMsg.classList.remove("hidden");
      return;
    }
    noSecretsMsg.classList.add("hidden");

    secrets.forEach(secret => {
      const li = document.createElement("li");
      li.className = "secret-item";
      
      const iconType = icons[secret.type] || icons.login;
      
      li.innerHTML = `
        <div class="secret-info">
          <div class="item-icon">${iconType}</div>
          <span class="secret-title">${escapeHtml(secret.title)}</span>
        </div>
        <svg class="arrow-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <polyline points="9 18 15 12 9 6"></polyline>
        </svg>
      `;

      li.addEventListener("click", () => {
        sendToHost({ action: "get", title: secret.title });
      });

      secretsList.appendChild(li);
    });
  }

  function showSecretDetails(secret) {
    detailTitle.innerText = secret.title;
    detailType.innerText = secret.type;
    detailSecretIcon.innerHTML = icons[secret.type] || icons.login;
    detailFieldsList.innerHTML = "";

    secret.fields.forEach(field => {
      const fieldDiv = document.createElement("div");
      fieldDiv.className = "detail-field";

      const label = document.createElement("span");
      label.className = "field-label";
      label.innerText = field.name;

      const row = document.createElement("div");
      row.className = "field-row";

      const valDisplay = document.createElement("div");
      valDisplay.className = "field-value-display";
      
      // Si la donnée est sensible, on l'affiche masquée à l'écran
      let isMasked = field.is_sensitive;
      valDisplay.innerText = isMasked ? "••••••••" : field.value;

      row.appendChild(valDisplay);

      // Bouton pour afficher/masquer (seulement si sensible)
      if (field.is_sensitive) {
        const toggleBtn = document.createElement("button");
        toggleBtn.className = "field-action-btn";
        toggleBtn.innerHTML = icons.eye;
        toggleBtn.title = "Afficher la valeur";
        toggleBtn.addEventListener("click", () => {
          isMasked = !isMasked;
          valDisplay.innerText = isMasked ? "••••••••" : field.value;
          toggleBtn.innerHTML = isMasked ? icons.eye : icons.eyeOff;
        });
        row.appendChild(toggleBtn);
      }

      // Bouton copier
      const copyBtn = document.createElement("button");
      copyBtn.className = "field-action-btn";
      copyBtn.innerHTML = icons.copy;
      copyBtn.title = "Copier la valeur";
      copyBtn.addEventListener("click", () => {
        navigator.clipboard.writeText(field.value).then(() => {
          // Micro-animation : confirmation de copie
          copyBtn.innerHTML = icons.check;
          copyBtn.style.color = "var(--success)";
          setTimeout(() => {
            copyBtn.innerHTML = icons.copy;
            copyBtn.style.color = "var(--text-muted)";
          }, 1500);
        });
      });

      row.appendChild(copyBtn);
      fieldDiv.appendChild(label);
      fieldDiv.appendChild(row);
      detailFieldsList.appendChild(fieldDiv);
    });

    searchSubview.classList.add("hidden");
    detailSubview.classList.remove("hidden");
  }

  // --- Gestion des Événements UI ---

  // Afficher / masquer le mot de passe maître saisi
  toggleMasterPwdBtn.addEventListener("click", () => {
    const isPwd = masterPasswordInput.type === "password";
    masterPasswordInput.type = isPwd ? "text" : "password";
    toggleMasterPwdBtn.innerHTML = isPwd ? icons.eyeOff : icons.eye;
  });

  if (toggleCreatePwdBtn && createPasswordInput) {
    toggleCreatePwdBtn.addEventListener("click", () => {
      const isPwd = createPasswordInput.type === "password";
      createPasswordInput.type = isPwd ? "text" : "password";
      toggleCreatePwdBtn.innerHTML = isPwd ? icons.eyeOff : icons.eye;
    });
  }

  if (toggleCreateConfirmPwdBtn && createConfirmPasswordInput) {
    toggleCreateConfirmPwdBtn.addEventListener("click", () => {
      const isPwd = createConfirmPasswordInput.type === "password";
      createConfirmPasswordInput.type = isPwd ? "text" : "password";
      toggleCreateConfirmPwdBtn.innerHTML = isPwd ? icons.eyeOff : icons.eye;
    });
  }

  // Action : Soumission du mot de passe maître pour déverrouiller
  unlockForm.addEventListener("submit", (e) => {
    e.preventDefault();
    const pwd = masterPasswordInput.value;
    if (!pwd) return;

    unlockBtn.disabled = true;
    unlockBtn.innerText = "Calcul (Argon2id)...";
    unlockError.classList.add("hidden");

    sendToHost({ action: "unlock", password: pwd });
  });

  // Action : Soumission du mot de passe maître pour créer le coffre
  if (createForm) {
    createForm.addEventListener("submit", (e) => {
      e.preventDefault();
      const pwd = createPasswordInput.value;
      const confirmPwd = createConfirmPasswordInput.value;
      if (!pwd) return;

      if (pwd !== confirmPwd) {
        showCreateError("Les mots de passe ne correspondent pas.");
        return;
      }

      createBtn.disabled = true;
      createBtn.innerText = "Calcul (Argon2id)...";
      if (createError) createError.classList.add("hidden");

      sendToHost({ action: "create", password: pwd });
    });
  }

  // Action : Recherche locale en temps réel
  searchInput.addEventListener("input", () => {
    const query = searchInput.value.toLowerCase().trim();
    if (!query) {
      renderSecretsList(allSecrets);
      return;
    }
    const filtered = allSecrets.filter(secret => 
      secret.title.toLowerCase().includes(query)
    );
    renderSecretsList(filtered);
  });

  // Action : Retour à la liste
  backToListBtn.addEventListener("click", () => {
    detailSubview.classList.add("hidden");
    searchSubview.classList.remove("hidden");
  });

  // Action : Bouton Lock
  lockBtn.addEventListener("lock", () => {
    sendToHost({ action: "lock" });
  });
  lockBtn.addEventListener("click", () => {
    sendToHost({ action: "lock" });
  });

  // --- Utilitaires ---
  function escapeHtml(str) {
    if (!str) return "";
    return str.replace(/&/g, "&amp;")
              .replace(/</g, "&lt;")
              .replace(/>/g, "&gt;")
              .replace(/"/g, "&quot;")
              .replace(/'/g, "&#039;");
  }

  // --- Lancement ---
  // Demander le statut courant du coffre au démarrage
  sendToHost({ action: "status" });
});