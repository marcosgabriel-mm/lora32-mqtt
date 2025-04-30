#ifndef HTML_DOCUMENTS_H
#define HTML_DOCUMENTS_H

static const char *WIFI_CONFIG_PAGE = 
"<!DOCTYPE html>"
"<html>"
"<head>"
"<link rel='shortcut icon' href='#'>"
"<title>Configurar WiFi</title>"
"<style>"
"body { font-family: Arial, sans-serif; background-color: #f0f0f0; margin: 0; padding: 0; display: flex; justify-content: center; align-items: center; height: 100vh; }"
"div { background-color: #fff; padding: 20px; border-radius: 8px; box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1); text-align: center; width: 90%; max-width: 400px; }"
"h1 { font-size: 24px; margin-bottom: 20px; color: #333; }"
"form { display: flex; flex-direction: column; gap: 10px; }"
"input[type='text'], input[type='password'] { padding: 10px; font-size: 16px; border: 1px solid #ccc; border-radius: 4px; width: 100%; box-sizing: border-box; }"
"input[type='submit'] { background-color: #007bff; color: #fff; border: none; padding: 10px; font-size: 16px; border-radius: 4px; cursor: pointer; }"
"input[type='submit']:hover { background-color: #0056b3; }"
"</style>"
"</head>"
"<body>"
"<div>"
"<h1>Configuração de WiFi</h1>"
"<form action='/set_wifi' method='get'>"
"  <input type='text' name='ssid' placeholder='SSID'><br>"
"  <input type='password' name='password' placeholder='Senha'><br>"
"  <input type='submit' value='Salvar'>"
"</form>"
"</div>"
"</body>"
"</html>";

static const char *WIFI_CONFIGURED_SUCCESSFUL = 
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"<link rel='shortcut icon' href='#'>\n"
"<title>WiFi Configurado</title>\n"
"<style>\n"
"  body { font-family: Arial, sans-serif; text-align: center; margin-top: 20%; }\n"
"  h1 { color: green; }\n"
"</style>\n"
"</head>\n"
"<body>\n"
"<h1>WiFi configurado com sucesso!</h1>\n"
"<p>Você já pode fechar esta página.</p>\n"
"</body>\n"
"</html>";

#endif