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
  const addBtn = document.getElementById("add-btn");
  const suggestionsSection = document.getElementById("suggestions-section");
  const suggestionsList = document.getElementById("suggestions-list");

  // Detail Subview Elements
  const searchSubview = document.getElementById("search-subview");
  const detailSubview = document.getElementById("detail-subview");
  const backToListBtn = document.getElementById("back-to-list-btn");
  const detailTitle = document.getElementById("detail-title");
  const detailType = document.getElementById("detail-type");
  const detailSecretIcon = document.getElementById("detail-secret-icon");
  const detailFieldsList = document.getElementById("detail-fields-list");
  const deleteSecretBtn = document.getElementById("delete-secret-btn");
  const fillSecretBtn = document.getElementById("fill-secret-btn");

  // Add Subview Elements
  const addSubview = document.getElementById("add-subview");
  const backToParentFromAddBtn = document.getElementById("back-to-list-from-add-btn");
  const addSecretForm = document.getElementById("add-secret-form");
  const addTitleInput = document.getElementById("add-title-input");
  const addTypeSelect = document.getElementById("add-type-select");
  const addFieldsContainer = document.getElementById("add-fields-container");
  const addCustomFieldBtn = document.getElementById("add-custom-field-btn");
  const addError = document.getElementById("add-error");

  // --- Session Data ---
  let allSecrets = []; // Tous les secrets chargés depuis le binaire
  let currentSecretTitle = ""; // Titre du secret sélectionné
  let currentSecretFields = null; // Champs du secret sélectionné pour le remplissage
  let deleteConfirmed = false;
  let activeFieldsList = []; // Liste des gestionnaires de champs actifs lors de l'ajout

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
        } else if (data.message === "Secret saved" || data.message === "Secret deleted") {
          // Retourne à la liste des secrets après ajout/suppression
          showSubview("search");
          sendToHost({ action: "list" });
          loadSuggestions(); // Recharger les suggestions après modification du coffre
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
        
        const activePanel = document.querySelector(".view-panel.active");
        if (activePanel && activePanel.id === "create-view") {
          showCreateError(data.message || "Une erreur est survenue.");
        } else if (addSubview && addSubview.classList.contains("active")) {
          showAddError(data.message || "Une erreur est survenue.");
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
      showSubview("search");
      loadSuggestions(); // Charger les suggestions dès le déverrouillage
    } else if (state === "no_vault") {
      if (createView) {
        createView.classList.remove("hidden");
        createView.classList.add("active");
      }
      statusDot.className = "dot locked";
      statusText.innerText = "Non initialisé";
      if (suggestionsSection) suggestionsSection.classList.add("hidden");
    } else { // "locked"
      lockedView.classList.remove("hidden");
      lockedView.classList.add("active");
      statusDot.className = "dot locked";
      statusText.innerText = "Verrouillé";
      showSubview("search");
      if (suggestionsSection) suggestionsSection.classList.add("hidden");
    }
  }

  function showSubview(subviewName) {
    searchSubview.classList.add("hidden");
    detailSubview.classList.add("hidden");
    if (addSubview) addSubview.classList.add("hidden");

    if (subviewName === "search") {
      searchSubview.classList.remove("hidden");
    } else if (subviewName === "detail") {
      detailSubview.classList.remove("hidden");
    } else if (subviewName === "add" && addSubview) {
      addSubview.classList.remove("hidden");
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

  function showAddError(msg) {
    if (addError) {
      addError.innerText = msg;
      addError.classList.remove("hidden");
    }
  }

  // Charge les suggestions correspondant au site visité sur l'onglet actif
  function loadSuggestions() {
    if (!suggestionsSection || !suggestionsList) return;

    browser.runtime.sendMessage({
      target: "background",
      action: "get-active-tab-suggestions"
    }).then(response => {
      suggestionsList.innerHTML = "";
      if (response && response.status === "success" && response.suggestions && response.suggestions.length > 0) {
        response.suggestions.forEach(s => {
          const li = document.createElement("li");
          li.className = "suggestion-item";
          
          li.innerHTML = `
            <div class="suggestion-info" style="cursor: pointer; flex: 1;">
              <div class="suggestion-info-header" style="display: flex; align-items: center; gap: 8px;">
                <div class="item-icon" style="width: 14px; height: 14px; color: var(--primary);">${icons[s.type] || icons.login}</div>
                <span class="suggestion-title" style="font-size: 13px; font-weight: 500;">${escapeHtml(s.title)}</span>
              </div>
              <span class="suggestion-user" style="font-size: 11px; color: var(--text-muted); padding-left: 22px;">${escapeHtml(s.username)}</span>
            </div>
            <button class="suggestion-fill-btn">
              Remplir
            </button>
          `;

          // Clic sur l'info ouvre les détails du secret
          li.querySelector(".suggestion-info").addEventListener("click", () => {
            showSecretDetails(s);
          });

          // Clic sur le bouton Remplir effectue l'injection et ferme la popup
          li.querySelector(".suggestion-fill-btn").addEventListener("click", (e) => {
            e.stopPropagation();
            browser.runtime.sendMessage({
              target: "background",
              action: "fill-credentials-on-page",
              fields: s.fields
            }).then(() => {
              window.close(); // Ferme la popup pour un rendu naturel
            });
          });

          suggestionsList.appendChild(li);
        });
        suggestionsSection.classList.remove("hidden");
      } else {
        suggestionsSection.classList.add("hidden");
      }
    }).catch(err => {
      console.error("Error loading suggestions:", err);
      suggestionsSection.classList.add("hidden");
    });
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
    currentSecretTitle = secret.title;
    currentSecretFields = secret.fields;
    detailTitle.innerText = secret.title;
    detailType.innerText = secret.type;
    detailSecretIcon.innerHTML = icons[secret.type] || icons.login;
    detailFieldsList.innerHTML = "";
    
    // Réinitialiser le bouton supprimer
    deleteConfirmed = false;
    deleteSecretBtn.innerHTML = `
      <svg class="btn-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
        <polyline points="3 6 5 6 21 6"></polyline>
        <path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2"></path>
      </svg>
      Supprimer
    `;
    deleteSecretBtn.classList.remove("btn-danger");

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

    showSubview("detail");
  }

  // --- Logique d'Ajout de Secret (Création manuelle) ---
  
  // Génère un mot de passe cryptographiquement sûr
  function generateSecurePassword(length = 16) {
    const chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()-_=+[]{}|;:,.<>?";
    let pwd = "";
    const randomValues = new Uint32Array(length);
    window.crypto.getRandomValues(randomValues);
    for (let i = 0; i < length; i++) {
      pwd += chars[randomValues[i] % chars.length];
    }
    return pwd;
  }

  // Ajoute un champ éditable dynamiquement dans la vue création
  function addFieldRowEdit(name = "", value = "", isSensitive = false, isDefault = false) {
    const fieldDiv = document.createElement("div");
    fieldDiv.className = "field-row-edit";
    
    const headerDiv = document.createElement("div");
    headerDiv.className = "field-header-edit";
    
    // Label ou input de nom de champ
    let nameGetter = null;
    if (isDefault) {
      const labelSpan = document.createElement("span");
      labelSpan.className = "field-label";
      labelSpan.innerText = name;
      headerDiv.appendChild(labelSpan);
      nameGetter = () => name;
    } else {
      const nameInput = document.createElement("input");
      nameInput.type = "text";
      nameInput.placeholder = "Nom du champ (ex: Code PIN)";
      nameInput.value = name;
      nameInput.required = true;
      nameInput.style.padding = "6px 10px";
      nameInput.style.fontSize = "12px";
      headerDiv.appendChild(nameInput);
      nameGetter = () => nameInput.value.trim();
      
      // Bouton supprimer le champ personnalisé
      const removeBtn = document.createElement("button");
      removeBtn.type = "button";
      removeBtn.className = "field-remove-btn";
      removeBtn.innerHTML = `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="3 6 5 6 21 6"></polyline><path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2"></path></svg>`;
      removeBtn.addEventListener("click", () => {
        fieldDiv.remove();
        activeFieldsList = activeFieldsList.filter(f => f.element !== fieldDiv);
      });
      headerDiv.appendChild(removeBtn);
    }
    fieldDiv.appendChild(headerDiv);

    // Wrapper pour la valeur
    const valWrapper = document.createElement("div");
    valWrapper.className = isSensitive ? "pwd-wrapper" : "input-wrapper";
    
    const valInput = document.createElement("input");
    valInput.type = isSensitive ? "password" : "text";
    valInput.placeholder = isSensitive ? "Saisie sécurisée" : "Valeur";
    valInput.value = value;
    valInput.required = true;
    valInput.style.padding = "10px 12px";
    valWrapper.appendChild(valInput);

    let currentSensitiveState = isSensitive;

    // Actions pour les champs sensibles (Afficher/Masquer et Générateur)
    function setupSensitiveActions() {
      // Supprime les anciennes actions si existantes
      const oldActions = valWrapper.querySelector(".pwd-actions");
      if (oldActions) oldActions.remove();

      if (currentSensitiveState) {
        const actionsDiv = document.createElement("div");
        actionsDiv.className = "pwd-actions";
        
        // Bouton afficher/masquer
        const toggleBtn = document.createElement("button");
        toggleBtn.type = "button";
        toggleBtn.className = "pwd-action-btn";
        toggleBtn.innerHTML = icons.eye;
        toggleBtn.addEventListener("click", () => {
          const isPwd = valInput.type === "password";
          valInput.type = isPwd ? "text" : "password";
          toggleBtn.innerHTML = isPwd ? icons.eyeOff : icons.eye;
        });
        actionsDiv.appendChild(toggleBtn);
        
        // Bouton générateur de mot de passe
        const genBtn = document.createElement("button");
        genBtn.type = "button";
        genBtn.className = "pwd-action-btn";
        genBtn.title = "Générer un mot de passe fort";
        genBtn.innerHTML = `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 2v2M12 20v2M4.93 4.93l1.41 1.41M17.66 17.66l1.41 1.41M2 12h2M20 12h2M6.34 17.66l-1.41 1.41M19.07 4.93l-1.41 1.41"></svg>`;
        genBtn.addEventListener("click", () => {
          valInput.value = generateSecurePassword(16);
          valInput.type = "text";
          toggleBtn.innerHTML = icons.eyeOff;
        });
        actionsDiv.appendChild(genBtn);

        valWrapper.appendChild(actionsDiv);
      }
    }

    setupSensitiveActions();
    fieldDiv.appendChild(valWrapper);

    // Checkbox sensibilité pour les champs personnalisés
    let sensitiveGetter = () => isSensitive;
    if (!isDefault) {
      const sensLabel = document.createElement("label");
      sensLabel.className = "sensitive-toggle";
      
      const sensCheck = document.createElement("input");
      sensCheck.type = "checkbox";
      sensCheck.checked = isSensitive;
      sensCheck.addEventListener("change", () => {
        currentSensitiveState = sensCheck.checked;
        valWrapper.className = currentSensitiveState ? "pwd-wrapper" : "input-wrapper";
        valInput.type = currentSensitiveState ? "password" : "text";
        setupSensitiveActions();
      });
      sensitiveGetter = () => sensCheck.checked;
      
      sensLabel.appendChild(sensCheck);
      sensLabel.appendChild(document.createTextNode(" Champ sensible (masqué/sécurisé)"));
      fieldDiv.appendChild(sensLabel);
    }

    addFieldsContainer.appendChild(fieldDiv);
    
    // Enregistre le gestionnaire de données du champ
    activeFieldsList.push({
      element: fieldDiv,
      getData: () => ({
        name: nameGetter(),
        value: valInput.value,
        is_sensitive: sensitiveGetter()
      })
    });
  }

  // Initialise les champs par défaut selon le type de secret
  function initDefaultFieldsForType(type) {
    addFieldsContainer.innerHTML = "";
    activeFieldsList = [];
    
    if (type === "login") {
      addFieldRowEdit("Identifiant", "", false, true);
      addFieldRowEdit("Mot de passe", "", true, true);
    } else if (type === "ssh") {
      addFieldRowEdit("Utilisateur", "", false, true);
      addFieldRowEdit("Clé Privée", "", true, true);
    } else if (type === "card") {
      addFieldRowEdit("Titulaire", "", false, true);
      addFieldRowEdit("Numéro de carte", "", false, true);
      addFieldRowEdit("Expiration", "", false, true);
      addFieldRowEdit("CVV", "", true, true);
    } else if (type === "note") {
      addFieldRowEdit("Note", "", true, true);
    }
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

  // Action : Retour à la liste (depuis détail)
  backToListBtn.addEventListener("click", () => {
    showSubview("search");
  });

  // Action : Retour à la liste (depuis création)
  if (backToParentFromAddBtn) {
    backToParentFromAddBtn.addEventListener("click", () => {
      showSubview("search");
    });
  }

  // Action : Bouton Lock
  lockBtn.addEventListener("click", () => {
    sendToHost({ action: "lock" });
  });

  // Action : Bouton Ajouter (affiche le formulaire de création)
  if (addBtn) {
    addBtn.addEventListener("click", () => {
      addTitleInput.value = "";
      addTypeSelect.value = "login";
      if (addError) addError.classList.add("hidden");
      initDefaultFieldsForType("login");
      showSubview("add");
    });
  }

  // Changement de type dans la création dynamique de secret
  if (addTypeSelect) {
    addTypeSelect.addEventListener("change", () => {
      initDefaultFieldsForType(addTypeSelect.value);
    });
  }

  // Ajout de champ personnalisé
  if (addCustomFieldBtn) {
    addCustomFieldBtn.addEventListener("click", () => {
      addFieldRowEdit("", "", false, false);
    });
  }

  // Soumission du formulaire d'enregistrement de secret
  if (addSecretForm) {
    addSecretForm.addEventListener("submit", (e) => {
      e.preventDefault();
      
      const title = addTitleInput.value.trim();
      const type = addTypeSelect.value;
      
      if (!title) {
        showAddError("Le titre est obligatoire.");
        return;
      }
      
      // Récupère les données de tous les champs
      const fields = activeFieldsList.map(f => f.getData());
      
      if (addError) addError.classList.add("hidden");
      
      sendToHost({
        action: "add",
        title: title,
        type: type,
        fields: fields
      });
    });
  }

  // Action : Remplissage à la demande depuis la popup
  if (fillSecretBtn) {
    fillSecretBtn.addEventListener("click", () => {
      if (currentSecretFields) {
        browser.runtime.sendMessage({
          target: "background",
          action: "fill-credentials-on-page",
          fields: currentSecretFields
        }).then(() => {
          window.close(); // Ferme la popup après remplissage
        }).catch(err => {
          console.error("Autofill from details failed:", err);
        });
      }
    });
  }

  // Action : Suppression d'un secret avec double-confirmation temporisée
  if (deleteSecretBtn) {
    deleteSecretBtn.addEventListener("click", () => {
      if (!deleteConfirmed) {
        deleteConfirmed = true;
        deleteSecretBtn.innerText = "Confirmer ?";
        deleteSecretBtn.classList.add("btn-danger");
        
        // Reset l'état après 3 secondes si l'utilisateur ne clique pas de nouveau
        setTimeout(() => {
          if (deleteConfirmed) {
            deleteConfirmed = false;
            deleteSecretBtn.innerHTML = `
              <svg class="btn-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <polyline points="3 6 5 6 21 6"></polyline>
                <path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2"></path>
              </svg>
              Supprimer
            `;
            deleteSecretBtn.classList.remove("btn-danger");
          }
        }, 3000);
      } else {
        // Exécute la suppression native
        sendToHost({
          action: "delete",
          title: currentSecretTitle
        });
        deleteConfirmed = false;
      }
    });
  }

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