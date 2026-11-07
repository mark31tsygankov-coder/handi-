document.addEventListener('submit', function (e) {
  var m = e.target.getAttribute && e.target.getAttribute('data-confirm');
  if (m && !confirm(m)) e.preventDefault();
});
