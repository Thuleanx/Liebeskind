module.exports = {
	responseFail(message, err) {
		return `Opps ${message.author}, something went wrong. Resulted with: ${err}`;
	}
}