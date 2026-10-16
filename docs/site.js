"use strict";

const themeNames = {
  mint: "Nox Mint",
  ocean: "Oceano",
  violet: "Violeta",
  ember: "Âmbar",
  daylight: "Claro",
};
const preview = document.querySelector("#theme-preview");
const screenshotLink = document.querySelector("#screenshot-link");
const caption = document.querySelector("#theme-caption");
const themeButtons = document.querySelectorAll("[data-theme]");

themeButtons.forEach((button) =>
  button.addEventListener("click", () => {
    const theme = button.dataset.theme;
    themeButtons.forEach((option) =>
      option.setAttribute("aria-pressed", String(option === button)),
    );
    preview.src = `assets/nox-${theme}.webp`;
    preview.alt = `Captura real do programa no tema ${themeNames[theme]}.`;
    screenshotLink.href = preview.src;
    screenshotLink.setAttribute(
      "aria-label",
      `Abrir captura do tema ${themeNames[theme]} em tamanho completo`,
    );
    caption.textContent = `${themeNames[theme]} · captura real no Windows`;
  }),
);

document.querySelector("#copy-command").addEventListener("click", async () => {
  const command = document.querySelector("#linux-command");
  const status = document.querySelector("#copy-status");
  try {
    await navigator.clipboard.writeText(command.textContent);
    status.textContent =
      "Comando copiado. Cole no terminal, na pasta do download.";
  } catch {
    const selection = window.getSelection();
    const range = document.createRange();
    range.selectNodeContents(command);
    selection.removeAllRanges();
    selection.addRange(range);
    status.textContent = "Comando selecionado. Use Ctrl+C para copiar.";
  }
});
