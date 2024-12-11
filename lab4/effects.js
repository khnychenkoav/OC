// effects.js

// Show a popup message
function showPopup(message) {
    const popup = document.querySelector('.popup');
    popup.textContent = message;
    popup.classList.add('show');

    setTimeout(() => {
        popup.classList.remove('show');
    }, 3000);
}

// Change the theme dynamically
function toggleTheme() {
    const body = document.body;
    if (body.classList.contains('dark-theme')) {
        body.classList.remove('dark-theme');
        body.style.background = 'linear-gradient(120deg, #f8f9fa, #e0e0e0)';
    } else {
        body.classList.add('dark-theme');
        body.style.background = 'linear-gradient(120deg, #1e3c72, #2a5298)';
    }
}
