const axios = require('axios');
const {retrieveToken} = require('../util/interact.js');
const {responseFail} = require('../util/responseFail.js');
const {prefix} = require('../appconfig.js');

const responseSuccess = () => {
	return `Your codeforces account is linked! Feel free to check your info using \`${prefix}self\` either in a server or in our dms.`;
}

module.exports = {
	name: 'linkCF',
	description: 'Link user with a handle on codeforces.com.',
	args: true,
	usage: '<cf_handle>',
	category: 'User',
	execute(message,args) {
		const {API_URL} = process.env;
		const username = message.author.username + "*" + message.author.discriminator;
		const cf_handle = args[0];

		retrieveToken().then(token => {
			axios.get(`${API_URL}/api/event/user/linkCF?username=${username}&handle=${cf_handle}`, {
				headers: {
					'Authentication': token
				}
			}).then(
				(_) => {
					message.channel.send(responseSuccess());
				},
				(err) => { 
					message.channel.send(responseFail(message, err.response.data.comment));
				}
			);
		}, (err) => { 
			message.channel.send(responseFail(message, err.response || `Seems like the server refuses to connect.`)); 
		});
	} 
}